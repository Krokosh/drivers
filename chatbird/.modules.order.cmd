cmd_/home/crok/drivers/chatbird/modules.order := {   echo /home/crok/drivers/chatbird/chatbird.ko; :; } | awk '!x[$$0]++' - > /home/crok/drivers/chatbird/modules.order
