# CMakeLists.txt for Domain library
cmake_minimum_required(VERSION 3.15)
project(Domain CXX)

add_library(Domain STATIC
    "include/Domain/Common/DomainConcepts.h"
    "include/Domain/Common/Result.h"
    "include/Domain/Common/ValidationError.h"
    "include/Domain/Common/Permissions.h"
    "include/Domain/Entities/Todo.h"
    "src/Entities/Todo.cpp"
    "include/Domain/Repositories/ITodoRepository.h"
    "include/Domain/Entities/CacheEntry.h"
    "include/Domain/Repositories/ICacheRepository.h"
    "include/Domain/Entities/QueueMessage.h"
    "include/Domain/Repositories/IMessageQueueRepository.h"
    "include/Domain/Entities/DocumentEntity.h"
    "include/Domain/Repositories/IDocumentRepository.h"
    "include/Domain/Entities/User.h"
    "include/Domain/Repositories/IUserRepository.h"
    "include/Domain/Entities/Session.h"
    "include/Domain/Repositories/ISessionRepository.h"
    "include/Domain/Entities/AuditLog.h"
    "include/Domain/Repositories/IAuditLogRepository.h"
)

target_compile_features(Domain PUBLIC cxx_std_20)

target_include_directories(Domain PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    "include/Domain/Common/DomainConcepts.h"
    "include/Domain/Common/Result.h"
    "include/Domain/Common/ValidationError.h"
    "include/Domain/Common/Permissions.h"
    "include/Domain/Entities/Todo.h"
    "src/Entities/Todo.cpp"
    "include/Domain/Repositories/ITodoRepository.h"
    "include/Domain/Entities/CacheEntry.h"
    "include/Domain/Repositories/ICacheRepository.h"
    "include/Domain/Entities/QueueMessage.h"
    "include/Domain/Repositories/IMessageQueueRepository.h"
    "include/Domain/Entities/DocumentEntity.h"
    "include/Domain/Repositories/IDocumentRepository.h"
    "include/Domain/Entities/User.h"
    "include/Domain/Repositories/IUserRepository.h"
    "include/Domain/Entities/Session.h"
    "include/Domain/Repositories/ISessionRepository.h"
    "include/Domain/Entities/AuditLog.h"
    "include/Domain/Repositories/IAuditLogRepository.h"
)
