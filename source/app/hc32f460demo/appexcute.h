/**
 * @file    appexcute.h
 * @brief   应用业务任务集合(1s周期任务的具体实现)
 * @note    任务在本文件实现, 由apptimer的1s任务表登记调度
 */
#ifndef APPEXCUTE_H
#define APPEXCUTE_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 1s心跳任务: 打印运行秒数(兼作SysTick存活探针) */
void AppExcuteHeartbeatTask(void);

#ifdef __cplusplus
}
#endif

#endif // APPEXCUTE_H
