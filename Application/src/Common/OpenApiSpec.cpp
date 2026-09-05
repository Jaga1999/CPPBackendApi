#include "Application/Common/OpenApiSpec.h"

namespace Application::Common {

std::string OpenApiSpec::generateJson() {
    return R"rawjson({
  "openapi": "3.1.0",
  "info": {
    "title": "CrowApi Multi-Paradigm Clean Architecture API",
    "version": "1.0.0",
    "description": "High-performance modern C++20 REST API using Crow framework and PostgreSQL 18.6 across 4 operational roles: Relational SQL Database, Redis Alternative (Key-Value Cache with TTL), Kafka Alternative (Message Queue with SKIP LOCKED), and MongoDB Alternative (Schemaless JSONB Document Store)."
  },
  "servers": [
    {
      "url": "http://localhost:8080",
      "description": "Local Development Server"
    }
  ],
  "tags": [
    { "name": "Health", "description": "Service health checks and uptime" },
    { "name": "Authentication", "description": "User registration, RS256 JWT login, refresh rotation, and session logout" },
    { "name": "JWKS", "description": "RFC 7517 JSON Web Key Set discovery for public asymmetric key verification" },
    { "name": "Sessions", "description": "User active multi-session management and device revocation" },
    { "name": "Admin Sessions", "description": "Administrative session tracking and global user revocation" },
    { "name": "Relational SQL (Todos)", "description": "ACID relational CRUD operations on PostgreSQL table" },
    { "name": "Redis Alternative (Cache)", "description": "High-performance key-value caching with TTL and eviction in PostgreSQL" },
    { "name": "Kafka Alternative (Queue)", "description": "Transactional message queuing and concurrent consumer polling via FOR UPDATE SKIP LOCKED" },
    { "name": "MongoDB Alternative (Documents)", "description": "Schemaless JSONB document collections with GIN indexing" },
    { "name": "Documentation", "description": "OpenAPI specification" }
  ],
  "paths": {
    "/api/health": {
      "get": {
        "tags": ["Health"],
        "summary": "Health check",
        "description": "Returns service health, version, uptime, and current timestamp.",
        "responses": {
          "200": { "description": "Service is healthy" }
        }
      }
    },
    "/api/todos": {
      "get": {
        "tags": ["Relational SQL (Todos)"],
        "summary": "List all todos",
        "parameters": [
          {
            "name": "completed",
            "in": "query",
            "required": false,
            "schema": { "type": "boolean" },
            "description": "Filter by completion status"
          }
        ],
        "responses": {
          "200": { "description": "List of todos retrieved successfully" }
        }
      },
      "post": {
        "tags": ["Relational SQL (Todos)"],
        "summary": "Create a new todo",
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": {
                "type": "object",
                "required": ["title"],
                "properties": {
                  "title": { "type": "string", "example": "Learn C++20" },
                  "description": { "type": "string", "example": "Explore concepts and ranges" }
                }
              }
            }
          }
        },
        "responses": {
          "201": { "description": "Todo created successfully" },
          "400": { "description": "Validation failed" }
        }
      }
    },
    "/api/todos/{id}": {
      "get": {
        "tags": ["Relational SQL (Todos)"],
        "summary": "Get todo by ID",
        "parameters": [
          { "name": "id", "in": "path", "required": true, "schema": { "type": "integer" } }
        ],
        "responses": {
          "200": { "description": "Todo found" },
          "404": { "description": "Todo not found" }
        }
      },
      "put": {
        "tags": ["Relational SQL (Todos)"],
        "summary": "Update todo",
        "parameters": [
          { "name": "id", "in": "path", "required": true, "schema": { "type": "integer" } }
        ],
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": {
                "type": "object",
                "properties": {
                  "title": { "type": "string" },
                  "description": { "type": "string" },
                  "completed": { "type": "boolean" }
                }
              }
            }
          }
        },
        "responses": {
          "200": { "description": "Todo updated successfully" },
          "404": { "description": "Todo not found" }
        }
      },
      "delete": {
        "tags": ["Relational SQL (Todos)"],
        "summary": "Delete todo",
        "parameters": [
          { "name": "id", "in": "path", "required": true, "schema": { "type": "integer" } }
        ],
        "responses": {
          "200": { "description": "Todo deleted" },
          "404": { "description": "Todo not found" }
        }
      }
    },
    "/api/cache/{key}": {
      "get": {
        "tags": ["Redis Alternative (Cache)"],
        "summary": "Get cache entry by key",
        "parameters": [
          { "name": "key", "in": "path", "required": true, "schema": { "type": "string" } }
        ],
        "responses": {
          "200": { "description": "Cache hit" },
          "404": { "description": "Cache miss or expired" }
        }
      },
      "delete": {
        "tags": ["Redis Alternative (Cache)"],
        "summary": "Evict cache entry by key",
        "parameters": [
          { "name": "key", "in": "path", "required": true, "schema": { "type": "string" } }
        ],
        "responses": {
          "200": { "description": "Evicted" },
          "404": { "description": "Key not found" }
        }
      }
    },
    "/api/cache": {
      "post": {
        "tags": ["Redis Alternative (Cache)"],
        "summary": "Set key-value in cache with TTL",
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": {
                "type": "object",
                "required": ["key", "value"],
                "properties": {
                  "key": { "type": "string", "example": "session:user:101" },
                  "value": { "type": "string", "example": "token_abc123" },
                  "ttlSeconds": { "type": "integer", "example": 3600 }
                }
              }
            }
          }
        },
        "responses": {
          "200": { "description": "Cache entry set" }
        }
      }
    },
    "/api/cache/cleanup": {
      "post": {
        "tags": ["Redis Alternative (Cache)"],
        "summary": "Purge all expired cache entries",
        "responses": {
          "200": { "description": "Expired entries purged" }
        }
      }
    },
    "/api/queue/publish": {
      "post": {
        "tags": ["Kafka Alternative (Queue)"],
        "summary": "Publish message to topic",
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": {
                "type": "object",
                "required": ["topic", "payload"],
                "properties": {
                  "topic": { "type": "string", "example": "order-events" },
                  "payload": { "type": "string", "example": "{\"orderId\": 99, \"amount\": 49.99}" }
                }
              }
            }
          }
        },
        "responses": {
          "201": { "description": "Message published with generated ID" }
        }
      }
    },
    "/api/queue/poll": {
      "post": {
        "tags": ["Kafka Alternative (Queue)"],
        "summary": "Poll next message with atomic worker lock",
        "description": "Atomically locks and returns the next pending message in the topic using FOR UPDATE SKIP LOCKED.",
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": {
                "type": "object",
                "required": ["topic"],
                "properties": {
                  "topic": { "type": "string", "example": "order-events" }
                }
              }
            }
          }
        },
        "responses": {
          "200": { "description": "Message locked and returned for processing" },
          "404": { "description": "No pending messages in topic" }
        }
      }
    },
    "/api/queue/ack/{id}": {
      "post": {
        "tags": ["Kafka Alternative (Queue)"],
        "summary": "Acknowledge message processing completion",
        "parameters": [
          { "name": "id", "in": "path", "required": true, "schema": { "type": "integer" } }
        ],
        "responses": {
          "200": { "description": "Message marked as COMPLETED" },
          "404": { "description": "Message not found" }
        }
      }
    },
    "/api/queue/metrics": {
      "get": {
        "tags": ["Kafka Alternative (Queue)"],
        "summary": "Get queue metrics across topics",
        "responses": {
          "200": { "description": "Queue counts grouped by topic and status" }
        }
      }
    },
    "/api/documents/{collection}": {
      "post": {
        "tags": ["MongoDB Alternative (Documents)"],
        "summary": "Insert schemaless JSON document into collection",
        "parameters": [
          { "name": "collection", "in": "path", "required": true, "schema": { "type": "string" } }
        ],
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": { "type": "object" }
            }
          }
        },
        "responses": {
          "201": { "description": "Document inserted with generated UUID" }
        }
      }
    },
    "/api/documents/{collection}/{id}": {
      "get": {
        "tags": ["MongoDB Alternative (Documents)"],
        "summary": "Get document by UUID",
        "parameters": [
          { "name": "collection", "in": "path", "required": true, "schema": { "type": "string" } },
          { "name": "id", "in": "path", "required": true, "schema": { "type": "string" } }
        ],
        "responses": {
          "200": { "description": "Document found" },
          "404": { "description": "Document not found" }
        }
      },
      "put": {
        "tags": ["MongoDB Alternative (Documents)"],
        "summary": "Update document by UUID",
        "parameters": [
          { "name": "collection", "in": "path", "required": true, "schema": { "type": "string" } },
          { "name": "id", "in": "path", "required": true, "schema": { "type": "string" } }
        ],
        "requestBody": {
          "required": true,
          "content": { "application/json": { "schema": { "type": "object" } } }
        },
        "responses": {
          "200": { "description": "Document updated" }
        }
      },
      "delete": {
        "tags": ["MongoDB Alternative (Documents)"],
        "summary": "Delete document by UUID",
        "parameters": [
          { "name": "collection", "in": "path", "required": true, "schema": { "type": "string" } },
          { "name": "id", "in": "path", "required": true, "schema": { "type": "string" } }
        ],
        "responses": {
          "200": { "description": "Document removed" }
        }
      }
    },
    "/api/documents/{collection}/query": {
      "post": {
        "tags": ["MongoDB Alternative (Documents)"],
        "summary": "Query documents using JSONB containment filter",
        "description": "Performs high-speed GIN index JSONB containment query (WHERE data @> filter).",
        "parameters": [
          { "name": "collection", "in": "path", "required": true, "schema": { "type": "string" } }
        ],
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": {
                "type": "object",
                "example": { "status": "active", "category": "books" }
              }
            }
          }
        },
        "responses": {
          "200": { "description": "Matching documents returned" }
        }
      }
    },
    "/.well-known/jwks.json": {
      "get": {
        "tags": ["JWKS"],
        "summary": "RFC 7517 JSON Web Key Set discovery",
        "description": "Returns public RSA keys used to verify RS256 JWT tokens.",
        "responses": {
          "200": { "description": "Active and rotated JWK public keys" }
        }
      }
    },
    "/api/v1/auth/register": {
      "post": {
        "tags": ["Authentication"],
        "summary": "Register new user account",
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": {
                "type": "object",
                "required": ["email", "password"],
                "properties": {
                  "email": { "type": "string", "format": "email", "example": "user@example.com" },
                  "password": { "type": "string", "format": "password", "example": "SecurePass123!" },
                  "displayName": { "type": "string", "example": "John Doe" }
                }
              }
            }
          }
        },
        "responses": {
          "201": { "description": "User created successfully" },
          "400": { "description": "Validation error" },
          "409": { "description": "Email already exists" }
        }
      }
    },
    "/api/v1/auth/login": {
      "post": {
        "tags": ["Authentication"],
        "summary": "Authenticate user and issue tokens",
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": {
                "type": "object",
                "required": ["email", "password"],
                "properties": {
                  "email": { "type": "string", "format": "email" },
                  "password": { "type": "string", "format": "password" },
                  "deviceName": { "type": "string", "example": "Chrome on Windows" },
                  "clientType": { "type": "string", "example": "browser" }
                }
              }
            }
          }
        },
        "responses": {
          "200": { "description": "Authentication successful, returns RS256 JWT access token and refresh token" },
          "401": { "description": "Invalid credentials or account locked" }
        }
      }
    },
    "/api/v1/auth/refresh": {
      "post": {
        "tags": ["Authentication"],
        "summary": "Rotate refresh token and issue new access token",
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": {
                "type": "object",
                "required": ["refreshToken"],
                "properties": {
                  "refreshToken": { "type": "string" }
                }
              }
            }
          }
        },
        "responses": {
          "200": { "description": "Tokens refreshed and rotated" },
          "401": { "description": "Invalid, expired, or reused refresh token (reuse revokes all user sessions)" }
        }
      }
    },
    "/api/v1/auth/logout": {
      "post": {
        "tags": ["Authentication"],
        "summary": "Revoke current session",
        "security": [{ "bearerAuth": [] }],
        "responses": {
          "200": { "description": "Session revoked successfully" },
          "401": { "description": "Unauthorized" }
        }
      }
    },
    "/api/v1/auth/logout-all": {
      "post": {
        "tags": ["Authentication"],
        "summary": "Revoke all active sessions for authenticated user",
        "security": [{ "bearerAuth": [] }],
        "responses": {
          "200": { "description": "All sessions revoked" },
          "401": { "description": "Unauthorized" }
        }
      }
    },
    "/api/v1/sessions": {
      "get": {
        "tags": ["Sessions"],
        "summary": "List all active sessions for authenticated user",
        "security": [{ "bearerAuth": [] }],
        "responses": {
          "200": { "description": "List of user sessions with current session flag" },
          "401": { "description": "Unauthorized" }
        }
      }
    },
    "/api/v1/sessions/{sessionId}": {
      "delete": {
        "tags": ["Sessions"],
        "summary": "Revoke specific session owned by user",
        "security": [{ "bearerAuth": [] }],
        "parameters": [
          { "name": "sessionId", "in": "path", "required": true, "schema": { "type": "string", "format": "uuid" } }
        ],
        "responses": {
          "200": { "description": "Session revoked" },
          "403": { "description": "Forbidden - Cannot revoke session owned by another user" },
          "404": { "description": "Session not found" }
        }
      }
    },
    "/api/v1/admin/sessions": {
      "get": {
        "tags": ["Admin Sessions"],
        "summary": "Search all system sessions (Admin only)",
        "security": [{ "bearerAuth": [] }],
        "parameters": [
          { "name": "userId", "in": "query", "schema": { "type": "string" } },
          { "name": "status", "in": "query", "schema": { "type": "string", "enum": ["active", "revoked", "expired"] } },
          { "name": "ipAddress", "in": "query", "schema": { "type": "string" } },
          { "name": "limit", "in": "query", "schema": { "type": "integer", "default": 50 } },
          { "name": "offset", "in": "query", "schema": { "type": "integer", "default": 0 } }
        ],
        "responses": {
          "200": { "description": "List of matched sessions" },
          "403": { "description": "Forbidden - Requires session:read:all permission" }
        }
      }
    },
    "/api/v1/admin/sessions/{sessionId}": {
      "delete": {
        "tags": ["Admin Sessions"],
        "summary": "Administrative revocation of any session",
        "security": [{ "bearerAuth": [] }],
        "parameters": [
          { "name": "sessionId", "in": "path", "required": true, "schema": { "type": "string", "format": "uuid" } }
        ],
        "responses": {
          "200": { "description": "Session revoked by administrator" },
          "403": { "description": "Forbidden - Requires session:revoke:all permission" }
        }
      }
    },
    "/api/v1/admin/users/{userId}/sessions/revoke-all": {
      "post": {
        "tags": ["Admin Sessions"],
        "summary": "Administrative revocation of all sessions for a user",
        "security": [{ "bearerAuth": [] }],
        "parameters": [
          { "name": "userId", "in": "path", "required": true, "schema": { "type": "string", "format": "uuid" } }
        ],
        "responses": {
          "200": { "description": "All sessions for target user revoked" },
          "403": { "description": "Forbidden - Requires session:revoke:all permission" }
        }
      }
    },
    "/api/openapi.json": {
      "get": {
        "tags": ["Documentation"],
        "summary": "OpenAPI 3.1.0 JSON specification",
        "responses": {
          "200": { "description": "OpenAPI JSON spec" }
        }
      }
    }
  },
  "components": {
    "securitySchemes": {
      "bearerAuth": {
        "type": "http",
        "scheme": "bearer",
        "bearerFormat": "JWT"
      }
    }
  }
})rawjson";
}

std::string OpenApiSpec::generateSwaggerHtml() {
    return R"rawhtml(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>CrowApi Swagger UI</title>
  <link rel="stylesheet" type="text/css" href="https://unpkg.com/swagger-ui-dist@5/swagger-ui.css" />
  <link rel="icon" type="image/png" href="https://unpkg.com/swagger-ui-dist@5/favicon-32x32.png" sizes="32x32" />
  <style>
    html { box-sizing: border-box; overflow: -moz-scrollbars-vertical; overflow-y: scroll; }
    *, *:before, *:after { box-sizing: inherit; }
    body { margin: 0; background: #fafafa; font-family: sans-serif; }
    .topbar { background-color: #1b1b1b !important; }
  </style>
</head>
<body>
  <div id="swagger-ui"></div>
  <script src="https://unpkg.com/swagger-ui-dist@5/swagger-ui-bundle.js" charset="UTF-8"></script>
  <script src="https://unpkg.com/swagger-ui-dist@5/swagger-ui-standalone-preset.js" charset="UTF-8"></script>
  <script>
    window.onload = function() {
      window.ui = SwaggerUIBundle({
        url: "/api/openapi.json",
        dom_id: '#swagger-ui',
        deepLinking: true,
        presets: [
          SwaggerUIBundle.presets.apis,
          SwaggerUIStandalonePreset
        ],
        plugins: [
          SwaggerUIBundle.plugins.DownloadUrl
        ],
        layout: "StandaloneLayout"
      });
    };
  </script>
</body>
</html>)rawhtml";
}

} // namespace Application::Common
