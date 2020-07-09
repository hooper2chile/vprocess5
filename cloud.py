'''
    Vprocess5 - UdeChile: Felipe Hooper

    Google Drive cloud for Bioreactor.
    Macos and rasbian version.
    Application for synchronization.
'''

import os, sys, time, datetime, logging

logging.basicConfig(filename='/home/pi/vprocess5/log/cloud.log', level=logging.INFO, format='%(asctime)s:%(levelname)s:%(message)s')



TIME_SYNC = 120#360#60 #3600 #sync for 3600 [s] = 1 [hr]
#ID = '1V3oyGsuhUjIVEZprf1AAbGK15HhcIr5Q'
ID = '1BSWSZiKaR8uysl4p1I-GtPwOox449Ddp'
time.sleep(15)

gdrive = '/home/pi/vprocess5/config/./my_gdrive' #gdrive-linux-rpi
DIR2   = '/home/pi/vprocess5/csv/'


while True:
    hora = time.strftime("Hora=%H:%M:%S__Fecha=%d-%m-%y")
    try:
        os.system( gdrive + ' sync upload ' + DIR2 + '.' + ' ' + ID)
        logging.info('sincronizado: ' + hora)
        f = open(DIR2+'gdrive_sync.txt','a+')
        f.write('sincronizado: ' + hora + ' \n')
        f.close()
        time.sleep(TIME_SYNC)

    except:
        logging.info('Fallo al subir a cloud:' + hora)
