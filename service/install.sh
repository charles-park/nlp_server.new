#!/bin/bash
systemctl disable odroid-nlp.service && sync

cp ./odroid-nlp.service /etc/systemd/system/ && sync

systemctl enable odroid-nlp.service && sync

systemctl restart odroid-nlp.service && sync