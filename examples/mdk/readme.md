# examples/mdk/ ——MDK(Keil) 工程根

本目录下存放所有MDK工程（本仓库唯一允许IDE工程的地方）。

## 结构

- `hc32f460demo/` ——HC32F460跑板验证工程（前后台架构）

## 工程内引用源码的相对路径（从本层算起）

```
Include Paths:
    ..\..\..\source\modules\fanctrl
    ..\..\..\source\port\hc32f460

源文件:
    ..\..\..\source\modules\fanctrl\fanctrl.c
```

注意：`Objects/`、`Listings/`、`*.uvguix.*` 已被根 `.gitignore` 排除，无需手动清理。
