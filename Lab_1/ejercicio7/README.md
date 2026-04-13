cd ~/esp/projects/ejercicio_7

git clone https://github.com/espressif/esp32-camera.git

editar los archivos:
esp32-camara/examples/main/camera_pinout.h
esp32-camara/examples/main/take_picture.c

cd examples/
source ~/.bashrc && get_esp32 && idf.py build && idf.py -p /dev/ttyXXXX flash monitor

(/dev/ttyXXXX: reemplazar XXXX por el puerto del ESP32)