##Die Librarys sind in dem "MyLibrarys" - Verzeichnis

##Zum Einrichten
git config --global user.name "Nick"
git config --global user.email "thorsten.hornick@gmail.com"
ssh-keygen -o -t rsa -b 4096 -C "thorsten.hornick@gmail.com"
#Inhalt von ~/.ssh/id_rsa.pub als ssh-Key in Gitlab eintragen

##Zum regelmäßigen Update
git add
git commit -m "Garagentuer .190 (OTA, SMQTT, Keypad (geht noch nicht))"
git push -u origin master
pause
"# PIO_Garagentuer" 
