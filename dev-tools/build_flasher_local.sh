#!/bin/bash
# cd ~
# curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash
cd ..
sudo ../../bin/act -W ./.github/workflows/build-RaceBox-ESP32-DEV.yml --rm --pull --artifact-server-path ./local_artifacts
sudo ../../bin/act -W ./.github/workflows/build-RFID-Label-Writer-ESP32-DEV.yml --rm --pull --artifact-server-path ./local_artifacts
sudo ../../bin/act -W ./.github/workflows/build-RaceBox-GhostCar-ESP32-C3-Poti.yml --rm --pull --artifact-server-path ./local_artifacts
sudo ../../bin/act -W ./.github/workflows/build-RaceBox-GhostCar-ESP32-C3-NoPoti.yml --rm --pull --artifact-server-path ./local_artifacts
sudo ../../bin/act -W ./.github/workflows/build-RaceBox-GhostCar-ESP32-S3-Poti.yml --rm --pull --artifact-server-path ./local_artifacts
sudo ../../bin/act -W ./.github/workflows/build-RaceBox-GhostCar-ESP32-S3-NoPoti.yml --rm --pull --artifact-server-path ./local_artifacts
sudo ../../bin/act -W ./.github/workflows/build-RaceBox-StartingLightsDisplay-ESP32-C6.yml --rm --pull --artifact-server-path ./local_artifacts
sudo chmod -R 777 ./local_artifacts
ls -lR ./local_artifacts
cd dev-tools