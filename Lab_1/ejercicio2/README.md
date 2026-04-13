cd ~/esp/projects/ejercicio_2
source ~/.bashrc && get_esp32 && idf.py build && idf.py -p /dev/ttyXXXX flash monitor

(/dev/ttyXXXX: reemplazar XXXX por el puerto del ESP32)

codigo colab: https://colab.research.google.com/drive/1fP1OqmerTvRWqFkQBbh2PtJ5X5EOupJr?authuser=3#scrollTo=sB5dGX_E6k4B