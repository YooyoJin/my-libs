# projects/gcc/——gcc/CMake构建根

本目录只放构建文件,应用源码在source/app/。构建产物输出到本目录的build/子目录(已被gitignore)。

## 使用(在仓库根执行)

```
cmake -S projects/gcc -B projects/gcc/build -G "MinGW Makefiles"
cmake --build projects/gcc/build
.\projects\gcc\build\fanctrldemo.exe
```

## 模块登记规则

每新增一个模块,在CMakeLists.txt的“模块库登记处”加一段add_library;此处与根README的模块清单表互为印证。