class timerCallback
{
private:
    unsigned long interval = 3000;
    unsigned long previousTime;
    void (*callback)() = nullptr;
public:
    timerCallback(void (*callbackFunc)(), unsigned long interval);
    void loop();
};