cd ~/esp/projects/estimacion
source ~/.bashrc && get_esp32 && idf.py build && idf.py -p /dev/ttyXXXX flash monitor

(/dev/ttyXXXX: reemplazar XXXX por el puerto del ESP32)

Link Collab: https://colab.research.google.com/drive/18lfCM-sg75XllHOz_QbxQAQ3fcrFvZ63?usp=sharing