#include <stdio.h>
#include <stdint.h>

#include <bluetooth/bluetooth.h>


static void ready_cb(int err)
{
    if (err) {
        LOG_DBG("Bluetooth enable failed: %d", err);
        return;
    }
    
    LOG_DBG("Bluetooth ready");
}

int main(void)
{
	LOG_DBG("Hello World!");

    bt_stack_init_once();

    bt_enable(ready_cb);

    LOG_DBG("Bluetooth enabled");

    while (1) {
        os_sleep_ms(1000);
    }

	return 0;
}
