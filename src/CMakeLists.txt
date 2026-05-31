# src/CMakeLists.txt
# 这个文件是 src 目录的“包工头”，负责按顺序引入各个子模块

# 1. 引入基础设施层 (Logger, Config)
add_subdirectory(common)

# 2. 引入核心业务层 (RecipeManager, Protobuf)
add_subdirectory(core)

# 3. 引入外部网关层 (TCP Server) - 暂时注释，Step 4 会加
add_subdirectory(gateway)

# 4. 引入协调层 - 暂时注释，Step 4 会加
add_subdirectory(controller)

# 5. 引入应用程序入口 (main.cpp)
# ⚠️ 注意：app 必须放在最后！因为它需要链接前面所有的库 (CommonLib, CoreLib)
add_subdirectory(app)