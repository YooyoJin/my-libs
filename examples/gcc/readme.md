# examples/gcc/ ——gcc/CMake构建根

本目录是CMake的**顶层入口**（本仓库不设根级CMakeLists）。

## 使用

```powershell
# 在仓库根执行
cmake -S examples/gcc -B build -G "MinGW Makefiles"
cmake --build build
.\build\pcdemo\fanctrldemo.exe
```

## 结构

- `CMakeLists.txt` ——顶层：模块库目标登记处（引用 `../../source/`）+ 挂接各demo
- `pcdemo/` ——PC仿真验证（假风机模型，无需板子）

## 模块登记规则

每新增一个模块，在 `CMakeLists.txt` 的"模块库登记处"加一段 `add_library`；
此处与根README的模块清单表互为印证。
