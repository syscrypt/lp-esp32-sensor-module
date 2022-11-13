idf_component_register(
	SRCS "transfer_protocol.c" "prot_wifi.c" "prot_zigbee.c" "prot_bluetooth.c" "prot_config.c" "user_reg_init_protocols.c"
	INCLUDE_DIRS "."
	REQUIRES bt util device command event_handler)
