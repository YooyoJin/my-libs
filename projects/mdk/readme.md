# projects/mdk/——MDK(Keil)工程根

本目录只放MDK工程文件(.uvprojx/.uvoptx),应用源码在source/app/。每个工程一个子目录,构建产物(Objects/Listings)落在工程目录内,已被gitignore。

## 工程内引用源码的相对路径(以projects/mdk/hc32f460demo/为例)

```
Include Paths:
    ..\..\..\source\modules\fanctrl
    ..\..\..\source\port\hc32f460

源文件:
    ..\..\..\source\modules\fanctrl\fanctrl.c
    ..\..\..\source\app\hc32f460demo\main.c
```