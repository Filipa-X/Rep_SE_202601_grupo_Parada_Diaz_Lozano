cd ~/esp/projects/ejercicio_5
idf.py add-dependency "espressif/esp-dsp"
source ~/.bashrc && get_esp32 && idf.py build && idf.py -p /dev/ttyXXXX flash monitor

(/dev/ttyXXXX: reemplazar XXXX por el puerto del ESP32)