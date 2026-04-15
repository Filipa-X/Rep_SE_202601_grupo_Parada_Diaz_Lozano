cd ~/esp/projects/ejercicio_2
source ~/.bashrc && get_esp32 && idf.py build && idf.py -p /dev/ttyXXXX flash monitor

(/dev/ttyXXXX: reemplazar XXXX por el puerto del ESP32)

codigo colab: https://colab.research.google.com/drive/1OCGf_f7Ar2ccRRMw5vuI4ef__JwmH8V6?usp=sharing