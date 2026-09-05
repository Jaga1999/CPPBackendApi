#include "Application/UseCases/AuthUseCases.h"
#include "Application/Validation/AuthInputValidator.h"
#include <chrono>

namespace Application::UseCases {

namespace {

std::string formatIsoTimestamp(std::chrono::system_clock::time_point tp) {
    const auto timeT = std::chrono::system_clock::to_time_t(tp);
    std::tm tmBuffer{};
#if defined(_WIN32)
    gmtime_s(&tmBuffer, &timeT);
#else
    gmtime_r(&timeT, &tmBuffer);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tmBuffer);
    return std::string(buf);
}

} // anonymous namespace

AuthUseCases::AuthUseCases(
    std::shared_ptr<Domain::Repositories::IUserRepository> userRepo,
    std::shared_ptr<Domain::Repositories::ISessionRepository> sessionRepo,
    std::shared_ptr<Domain::Repositories::IAuditLogRepository> auditRepo,
    std::shared_ptr<Security::IJwtService> jwtService,
    std::shared_ptr<Security::IPasswordHasher> passwordHasher,
    std::shared_ptr<Security::ITokenGenerator> tokenGenerator,
    std::shared_ptr<Security::IGoogleAuthService> googleAuthService,
    std::shared_ptr<Domain::Repositories::ICacheRepository> cacheRepo
)   : m_userRepo(std::move(userRepo)),
      m_sessionRepo(std::move(sessionRepo)),
      m_auditRepo(std::move(auditRepo)),
      m_jwtService(std::move(jwtService)),
      m_passwordHasher(std::move(passwordHasher)),
      m_tokenGenerator(std::move(tokenGenerator)),
      m_googleAuthService(std::move(googleAuthService)),
      m_cacheRepo(std::move(cacheRepo)) {}

Domain::Common::Result<Common::ApiResponse<DTOs::RegisterResponse>> AuthUseCases::registerUser(
    const DTOs::RegisterRequest& request
) {
    auto valRes = Validation::AuthInputValidator::validateRegister(request);
    if (valRes.isErr()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::RegisterResponse>>::err(valRes.error());
    }

    auto existingUser = m_userRepo->findByEmail(request.email);
    if (existingUser.has_value()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::RegisterResponse>>::err(
            Domain::Common::DomainError{
                .message = "Registration conflict",
                .statusCode = 409,
                .details = {"An account with this email address already exists."}
            }
        );
    }

    std::string hashedPassword = m_passwordHasher->hashPassword(request.password);
    Domain::Entities::User newUser{
        .email = request.email,
        .passwordHash = std::move(hashedPassword),
        .role = request.role.empty() ? "user" : request.role,
        .isActive = true,
        .failedLoginAttempts = 0
    };

    auto createdUser = m_userRepo->create(std::move(newUser));
    if (createdUser.id.empty()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::RegisterResponse>>::err(
            Domain::Common::DomainError{.message = "User creation error", .statusCode = 500, .details = {"Failed to insert user record."}}
        );
    }

    m_auditRepo->record(Domain::Entities::AuditLog{
        .eventType = "USER_REGISTERED",
        .userId = createdUser.id,
        .reason = "User registered with role " + createdUser.role
    });

    DTOs::RegisterResponse resp{
        .id = createdUser.id,
        .email = createdUser.email,
        .role = createdUser.role,
        .createdAt = formatIsoTimestamp(createdUser.createdAt)
    };

    return Domain::Common::Result<Common::ApiResponse<DTOs::RegisterResponse>>::ok(
        Common::ApiResponse<DTOs::RegisterResponse>::created(std::move(resp), "User registered successfully")
    );
}

Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>> AuthUseCases::login(
    const DTOs::LoginRequest& request,
    std::string_view ipAddress,
    std::string_view userAgent,
    std::string_view deviceName,
    std::string_view clientType
) {
    auto valRes = Validation::AuthInputValidator::validateLogin(request);
    if (valRes.isErr()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(valRes.error());
    }

    auto userOpt = m_userRepo->findByEmail(request.email);
    if (!userOpt.has_value()) {
        m_auditRepo->record(Domain::Entities::AuditLog{
            .eventType = "LOGIN_FAILURE",
            .ipAddress = std::string(ipAddress),
            .userAgent = std::string(userAgent),
            .reason = "Unknown user email: " + request.email
        });
        return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(
            Domain::Common::DomainError{.message = "Authentication failed", .statusCode = 401, .details = {"Invalid email or password."}}
        );
    }

    auto user = *userOpt;
    if (!user.isActive) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(
            Domain::Common::DomainError{.message = "Account deactivated", .statusCode = 403, .details = {"This user account has been disabled."}}
        );
    }

    const auto now = std::chrono::system_clock::now();
    if (user.lockedUntil.has_value() && now < *user.lockedUntil) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(
            Domain::Common::DomainError{
                .message = "Account temporarily locked",
                .statusCode = 429,
                .details = {"Account locked due to excessive failed attempts. Please try again later."}
            }
        );
    }

    if (!user.passwordHash.has_value() || user.passwordHash->empty()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(
            Domain::Common::DomainError{
                .message = "Password login unavailable",
                .statusCode = 400,
                .details = {"This account was registered via Google Sign-In. Please sign in with Google, or add a password using the set-password endpoint."}
            }
        );
    }

    bool passwordMatches = m_passwordHasher->verifyPassword(request.password, *user.passwordHash);
    if (!passwordMatches) {
        m_userRepo->incrementFailedAttempts(user.id);
        if (user.failedLoginAttempts + 1 >= 5) {
            m_userRepo->lockAccount(user.id, now + std::chrono::minutes(15));
            m_auditRepo->record(Domain::Entities::AuditLog{
                .eventType = "ACCOUNT_LOCKED",
                .userId = user.id,
                .ipAddress = std::string(ipAddress),
                .userAgent = std::string(userAgent),
                .reason = "Account locked for 15 minutes after 5 failed attempts."
            });
        }
        m_auditRepo->record(Domain::Entities::AuditLog{
            .eventType = "LOGIN_FAILURE",
            .userId = user.id,
            .ipAddress = std::string(ipAddress),
            .userAgent = std::string(userAgent),
            .reason = "Incorrect password provided."
        });
        return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(
            Domain::Common::DomainError{.message = "Authentication failed", .statusCode = 401, .details = {"Invalid email or password."}}
        );
    }

    // Password verified: reset failure count
    m_userRepo->resetFailedAttempts(user.id);

    // Create session & tokens
    std::string jti = m_tokenGenerator->generateUuid();
    std::string rawRefreshToken = m_tokenGenerator->generateSecureToken(32);
    std::string refreshTokenHash = m_tokenGenerator->sha256Hex(rawRefreshToken);
    auto sessionExpiry = now + std::chrono::hours(24 * 7); // 7-day refresh lifetime

    Domain::Entities::Session session{
        .userId = user.id,
        .jti = jti,
        .refreshTokenHash = refreshTokenHash,
        .createdAt = now,
        .lastSeenAt = now,
        .expiresAt = sessionExpiry,
        .ipAddress = std::string(ipAddress),
        .userAgent = std::string(userAgent),
        .deviceName = std::string(deviceName),
        .clientType = std::string(clientType)
    };

    auto createdSession = m_sessionRepo->createSession(std::move(session));
    if (createdSession.id.empty()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(
            Domain::Common::DomainError{.message = "Session error", .statusCode = 500, .details = {"Failed to persist user session."}}
        );
    }

    constexpr std::chrono::seconds accessTokenTtl{900}; // 15-minute access token
    std::string accessToken = m_jwtService->createAccessToken(
        user.id,
        user.role,
        createdSession.id,
        jti,
        accessTokenTtl
    );

    m_auditRepo->record(Domain::Entities::AuditLog{
        .eventType = "LOGIN_SUCCESS",
        .userId = user.id,
        .sessionId = createdSession.id,
        .ipAddress = std::string(ipAddress),
        .userAgent = std::string(userAgent),
        .reason = "Session initiated on device: " + std::string(deviceName)
    });

    DTOs::TokenResponse resp{
        .accessToken = std::move(accessToken),
        .refreshToken = std::move(rawRefreshToken),
        .tokenType = "Bearer",
        .expiresIn = accessTokenTtl.count(),
        .sessionId = createdSession.id,
        .user = DTOs::UserDto{
            .id = user.id,
            .email = user.email,
            .role = user.role,
            .authProvider = user.authProvider,
            .avatarUrl = user.avatarUrl.value_or(""),
            .googleLinked = user.googleId.has_value()
        }
    };

    return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::ok(
        Common::ApiResponse<DTOs::TokenResponse>::ok(std::move(resp), "Login successful")
    );
}

Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>> AuthUseCases::refresh(
    const DTOs::RefreshTokenRequest& request,
    std::string_view ipAddress,
    std::string_view userAgent
) {
    auto valRes = Validation::AuthInputValidator::validateRefreshToken(request);
    if (valRes.isErr()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(valRes.error());
    }

    std::string tokenHash = m_tokenGenerator->sha256Hex(request.refreshToken);
    auto sessionOpt = m_sessionRepo->findByRefreshTokenHash(tokenHash);

    if (!sessionOpt.has_value() || sessionOpt->isRevoked()) {
        if (sessionOpt.has_value() && sessionOpt->isRevoked()) {
            // Token Reuse Detected!
            m_sessionRepo->revokeAllUserSessions(sessionOpt->userId, "Security: Refresh token reuse detected");
            m_auditRepo->record(Domain::Entities::AuditLog{
                .eventType = "REFRESH_REUSE_DETECTED",
                .userId = sessionOpt->userId,
                .sessionId = sessionOpt->id,
                .ipAddress = std::string(ipAddress),
                .userAgent = std::string(userAgent),
                .reason = "Previously revoked refresh token was re-presented. All sessions revoked."
            });
        }
        return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(
            Domain::Common::DomainError{.message = "Invalid refresh token", .statusCode = 401, .details = {"Token is invalid, expired, or revoked."}}
        );
    }

    auto session = *sessionOpt;
    const auto now = std::chrono::system_clock::now();
    if (session.isExpired(now)) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(
            Domain::Common::DomainError{.message = "Session expired", .statusCode = 401, .details = {"The refresh session has expired. Please log in again."}}
        );
    }

    auto userOpt = m_userRepo->findById(session.userId);
    if (!userOpt.has_value() || !userOpt->isActive) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(
            Domain::Common::DomainError{.message = "User inactive", .statusCode = 401, .details = {"Account is inactive or disabled."}}
        );
    }

    // Refresh Token Rotation: generate new refresh token and invalidate old one
    std::string newRawToken = m_tokenGenerator->generateSecureToken(32);
    std::string newHash = m_tokenGenerator->sha256Hex(newRawToken);
    std::string newJti = m_tokenGenerator->generateUuid();
    auto newExpiry = now + std::chrono::hours(24 * 7);

    bool updated = m_sessionRepo->updateRefreshToken(session.id, newHash, newJti, newExpiry);
    if (!updated) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(
            Domain::Common::DomainError{.message = "Refresh collision", .statusCode = 409, .details = {"Concurrent refresh conflict detected."}}
        );
    }

    constexpr std::chrono::seconds accessTokenTtl{900};
    std::string newAccessToken = m_jwtService->createAccessToken(
        userOpt->id,
        userOpt->role,
        session.id,
        newJti,
        accessTokenTtl
    );

    m_auditRepo->record(Domain::Entities::AuditLog{
        .eventType = "REFRESH_SUCCESS",
        .userId = userOpt->id,
        .sessionId = session.id,
        .ipAddress = std::string(ipAddress),
        .userAgent = std::string(userAgent),
        .reason = "Refresh token rotated successfully"
    });

    DTOs::TokenResponse resp{
        .accessToken = std::move(newAccessToken),
        .refreshToken = std::move(newRawToken),
        .tokenType = "Bearer",
        .expiresIn = accessTokenTtl.count(),
        .sessionId = session.id,
        .user = DTOs::UserDto{
            .id = userOpt->id,
            .email = userOpt->email,
            .role = userOpt->role,
            .authProvider = userOpt->authProvider,
            .avatarUrl = userOpt->avatarUrl.value_or(""),
            .googleLinked = userOpt->googleId.has_value()
        }
    };

    return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::ok(
        Common::ApiResponse<DTOs::TokenResponse>::ok(std::move(resp), "Token refreshed successfully")
    );
}

Domain::Common::Result<Common::ApiResponse<void>> AuthUseCases::logout(
    std::string_view sessionId,
    std::string_view userId,
    std::string_view ipAddress,
    std::string_view userAgent
) {
    bool revoked = m_sessionRepo->revokeSession(sessionId, "User logout");
    if (!revoked) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{.message = "Session not found", .statusCode = 404, .details = {"Session does not exist or was already revoked."}}
        );
    }

    m_auditRepo->record(Domain::Entities::AuditLog{
        .eventType = "LOGOUT",
        .userId = std::string(userId),
        .sessionId = std::string(sessionId),
        .ipAddress = std::string(ipAddress),
        .userAgent = std::string(userAgent),
        .reason = "User logged out current session."
    });

    return Domain::Common::Result<Common::ApiResponse<void>>::ok(
        Common::ApiResponse<void>::ok("Session terminated successfully")
    );
}

Domain::Common::Result<Common::ApiResponse<void>> AuthUseCases::logoutAll(
    std::string_view userId,
    std::string_view ipAddress,
    std::string_view userAgent
) {
    size_t count = m_sessionRepo->revokeAllUserSessions(userId, "User initiated logout-all");

    m_auditRepo->record(Domain::Entities::AuditLog{
        .eventType = "ALL_SESSIONS_REVOKED",
        .userId = std::string(userId),
        .ipAddress = std::string(ipAddress),
        .userAgent = std::string(userAgent),
        .reason = "User revoked all active sessions. Total sessions revoked: " + std::to_string(count)
    });

    return Domain::Common::Result<Common::ApiResponse<void>>::ok(
        Common::ApiResponse<void>::ok("All sessions terminated successfully. Count: " + std::to_string(count))
    );
}

Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>> AuthUseCases::loginWithGoogle(
    const DTOs::GoogleLoginRequest& request,
    std::string_view ipAddress,
    std::string_view userAgent
) {
    if (!m_googleAuthService) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(
            Domain::Common::DomainError{
                .message = "Google authentication unavailable",
                .statusCode = 501,
                .details = {"Google OAuth2 service is not enabled on this server."}
            }
        );
    }

    Domain::Common::Result<Security::GoogleUserInfo> googleResult = Domain::Common::Result<Security::GoogleUserInfo>::err(
        Domain::Common::DomainError{.message = "Missing token", .statusCode = 400, .details = {"Provide idToken or code."}}
    );

    if (!request.idToken.empty()) {
        googleResult = m_googleAuthService->verifyIdToken(request.idToken);
    } else if (!request.code.empty()) {
        std::string effectiveVerifier = request.codeVerifier;

        // If client did not explicitly provide codeVerifier, attempt to resolve from cached PKCE state
        if (effectiveVerifier.empty() && !request.state.empty() && m_cacheRepo) {
            auto cached = m_cacheRepo->get("pkce:state:" + request.state);
            if (cached.has_value() && !cached->isExpired()) {
                effectiveVerifier = cached->value;
                // Single-use token: purge state from cache immediately to prevent replay attacks
                m_cacheRepo->remove("pkce:state:" + request.state);
            } else {
                return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(
                    Domain::Common::DomainError{
                        .message = "Invalid or expired state",
                        .statusCode = 401,
                        .details = {"State parameter is invalid, forged, or has expired."}
                    }
                );
            }
        }

        googleResult = m_googleAuthService->exchangeAuthCode(request.code, effectiveVerifier);
    }

    if (googleResult.isErr()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(googleResult.error());
    }

    const auto& googleInfo = googleResult.value();
    auto now = std::chrono::system_clock::now();

    // 1. Check if user with this google_id exists
    std::optional<Domain::Entities::User> targetUser = m_userRepo->findByGoogleId(googleInfo.googleId);

    if (!targetUser.has_value()) {
        // 2. Check if user with matching email already exists -> BIDIRECTIONAL ACCOUNT LINKING
        auto userByEmail = m_userRepo->findByEmail(googleInfo.email);
        if (userByEmail.has_value()) {
            m_userRepo->linkGoogleAccount(userByEmail->id, googleInfo.googleId, googleInfo.picture);
            m_auditRepo->record(Domain::Entities::AuditLog{
                .eventType = "AUTH_GOOGLE_LINKED",
                .userId = userByEmail->id,
                .ipAddress = std::string(ipAddress),
                .userAgent = std::string(userAgent),
                .reason = "Linked Google account to existing user with matching email: " + googleInfo.email
            });
            targetUser = m_userRepo->findById(userByEmail->id);
        } else {
            // 3. New user registration via Google
            Domain::Entities::User newUser{
                .email = googleInfo.email,
                .passwordHash = std::nullopt,
                .role = "user",
                .isActive = true,
                .failedLoginAttempts = 0,
                .googleId = googleInfo.googleId,
                .authProvider = "google",
                .avatarUrl = googleInfo.picture.empty() ? std::nullopt : std::make_optional(googleInfo.picture)
            };
            auto created = m_userRepo->create(std::move(newUser));
            if (created.id.empty()) {
                return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(
                    Domain::Common::DomainError{.message = "Google signup error", .statusCode = 500, .details = {"Failed to create user record."}}
                );
            }
            m_auditRepo->record(Domain::Entities::AuditLog{
                .eventType = "AUTH_GOOGLE_REGISTER",
                .userId = created.id,
                .ipAddress = std::string(ipAddress),
                .userAgent = std::string(userAgent),
                .reason = "New user registered via Google Sign-In."
            });
            targetUser = std::move(created);
        }
    }

    if (!targetUser.has_value() || !targetUser->isActive) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(
            Domain::Common::DomainError{.message = "Account disabled", .statusCode = 403, .details = {"User account is deactivated."}}
        );
    }

    const auto& user = *targetUser;
    m_userRepo->resetFailedAttempts(user.id);

    // Create session & tokens
    std::string jti = m_tokenGenerator->generateUuid();
    std::string rawRefreshToken = m_tokenGenerator->generateSecureToken(32);
    std::string refreshTokenHash = m_tokenGenerator->sha256Hex(rawRefreshToken);
    auto sessionExpiry = now + std::chrono::hours(24 * 30);

    Domain::Entities::Session session{
        .userId = user.id,
        .jti = jti,
        .refreshTokenHash = refreshTokenHash,
        .createdAt = now,
        .lastSeenAt = now,
        .expiresAt = sessionExpiry,
        .ipAddress = std::string(ipAddress),
        .userAgent = std::string(userAgent),
        .deviceName = request.deviceName.empty() ? "Google Client" : request.deviceName,
        .clientType = request.clientType.empty() ? "browser" : request.clientType
    };

    auto createdSession = m_sessionRepo->createSession(std::move(session));
    if (createdSession.id.empty()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::err(
            Domain::Common::DomainError{.message = "Session error", .statusCode = 500, .details = {"Failed to persist user session."}}
        );
    }

    constexpr std::chrono::seconds accessTokenTtl{900};
    std::string accessToken = m_jwtService->createAccessToken(
        user.id,
        user.role,
        createdSession.id,
        jti,
        accessTokenTtl
    );

    m_auditRepo->record(Domain::Entities::AuditLog{
        .eventType = "AUTH_GOOGLE_SUCCESS",
        .userId = user.id,
        .sessionId = createdSession.id,
        .ipAddress = std::string(ipAddress),
        .userAgent = std::string(userAgent),
        .reason = "Session initiated via Google OAuth2/OIDC"
    });

    DTOs::TokenResponse resp{
        .accessToken = std::move(accessToken),
        .refreshToken = std::move(rawRefreshToken),
        .tokenType = "Bearer",
        .expiresIn = accessTokenTtl.count(),
        .sessionId = createdSession.id,
        .user = DTOs::UserDto{
            .id = user.id,
            .email = user.email,
            .role = user.role,
            .authProvider = user.authProvider,
            .avatarUrl = user.avatarUrl.value_or(""),
            .googleLinked = user.googleId.has_value()
        }
    };

    return Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>>::ok(
        Common::ApiResponse<DTOs::TokenResponse>::ok(std::move(resp), "Google authentication successful", 200)
    );
}

Domain::Common::Result<Common::ApiResponse<DTOs::GoogleAuthUrlResponse>> AuthUseCases::getGoogleAuthUrl(
    std::string_view state
) {
    if (!m_googleAuthService) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::GoogleAuthUrlResponse>>::err(
            Domain::Common::DomainError{.message = "Google service not configured", .statusCode = 501, .details = {"Google OAuth2 is not enabled."}}
        );
    }
    std::string finalState = state.empty() ? m_tokenGenerator->generateSecureToken(16) : std::string(state);

    // RFC 7636 PKCE: Generate high-entropy 64-char verifier and SHA-256 Base64URL challenge
    std::string codeVerifier = m_googleAuthService->generateCodeVerifier();
    std::string codeChallenge = m_googleAuthService->generateCodeChallenge(codeVerifier);

    // Cache the PKCE verifier by state for 10 minutes (600s) for server-side verification upon callback
    if (m_cacheRepo) {
        m_cacheRepo->set("pkce:state:" + finalState, codeVerifier, 600);
    }

    std::string authUrl = m_googleAuthService->getAuthorizationUrl(finalState, codeChallenge);
    return Domain::Common::Result<Common::ApiResponse<DTOs::GoogleAuthUrlResponse>>::ok(
        Common::ApiResponse<DTOs::GoogleAuthUrlResponse>::ok(
            DTOs::GoogleAuthUrlResponse{
                .authUrl = std::move(authUrl),
                .state = std::move(finalState),
                .codeVerifier = std::move(codeVerifier),
                .codeChallenge = std::move(codeChallenge)
            },
            "Google authorization URL generated with RFC 7636 PKCE",
            200
        )
    );
}

Domain::Common::Result<Common::ApiResponse<void>> AuthUseCases::setPassword(
    std::string_view userId,
    const DTOs::SetPasswordRequest& request
) {
    if (request.password.length() < 8) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{
                .message = "Validation failed",
                .statusCode = 400,
                .details = {"Password must be at least 8 characters long."}
            }
        );
    }

    auto user = m_userRepo->findById(userId);
    if (!user.has_value()) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{.message = "User not found", .statusCode = 404, .details = {"User does not exist."}}
        );
    }

    std::string hashed = m_passwordHasher->hashPassword(request.password);
    bool success = m_userRepo->setPassword(userId, hashed);
    if (!success) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{.message = "Database error", .statusCode = 500, .details = {"Failed to update password."}}
        );
    }

    m_auditRepo->record(Domain::Entities::AuditLog{
        .eventType = "PASSWORD_SET",
        .userId = std::string(userId),
        .reason = "Password added to account, enabling dual password and Google login."
    });

    return Domain::Common::Result<Common::ApiResponse<void>>::ok(
        Common::ApiResponse<void>::ok("Password set successfully", 200)
    );
}

} // namespace Application::UseCases
