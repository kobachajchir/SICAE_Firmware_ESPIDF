
#ifdef __cplusplus
extern "C"
{
#endif
    void initIR();
    void logsActiveIRProtocols();
    void initIRTask(void *pvParameters);
    void irReceiveTask(void *pvParameters);
    void handleOverflow();
    bool detectLongPress(uint16_t aLongPressDurationMillis);
    void longPressTask(void *pvParameters);
#ifdef __cplusplus
}
#endif
