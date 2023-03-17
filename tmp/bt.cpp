#include "bt.h"

bool bt_startBT(){
	log_d("%s", __func__);
	esp_err_t ret;
    esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED){
    	log_i("BT controller already enabled");
        return true;
    }
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE){
    	log_i("BT status IDLE, initializing BT controller");
        ret = esp_bt_controller_init(&cfg);
        if (ret != ESP_OK) {
	        log_e("Bluetooth Controller initialize failed: %s", esp_err_to_name(ret));
    	    return false;
   		 }
        while(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE){
        	log_i("waiting for BT controller init");
        }
    }
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED){
        if (esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) {
            log_e("BT Enable failed");
            return false;
        }
    }
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED){
        return true;
    }
    log_e("BT Start failed");
    return false;

}

void bt_initNVS(){
	log_d("%s", __func__);
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND){
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK( err );
}

bool bt_initBT(){
	log_d("%s", __func__);
	if(!bt_startBT()){
		log_e("Failed to initialized controller");
		return false;
	}
}


void bt_init(){
	bt_initNVS();

	bt_initBT();
}