## OCF HCP DEVICES

This package includes several simulators for defined OCF devices for the Healthcare vertical.
It currently includes Blood Pressure Monitor, Glucose Meter, Body Temperature, and Body Weight (scale).
These simulators make use of Atomic Measurement implementation INSIDE of Iotivity.

For all reference about OCF Healthcare vertical, and other OCF inquiries, please consult: https://openconnectivity.org/ (OCF website).
For all reference about Iotivity, please consult: https://iotivity.org/ (Iotivity website).

This code passes all Atomic Measurement Test Cases (CT1.2.15 ~ CT1.2.20).

OS: Ubuntu 16.04 LTS

## Installation Guide
1. Clone this repository (git clone https://github.com/hjourdain/OCF-HCP-DEVICES
2. Clone iotivity inside OCF-HCP-DEVICES/Iotivity (cd OCF-HCP-DEVICES; git clone https://gerrit.iotivity.org/gerrit/p/iotivity.git Iotivity)
3. Type ./buildall.sh to build Iotivity and all the simulators
3.1. To build individual simulators, go to each simulator's directory, and type ./build.sh
4. To run an individual simulator, go to the simulator's directory, and type ./start.sh
5. If the CTT prompts "Please initiate device to revert to read for OTM", stop the simulator, restart it, and then press OK (on the CTT)

## Supported commands
Those simulators are interactive and will accept commands.
To QUIT any simulator, type 'q'.
To get HELP on the commands supported by the simulator, type 'h'.


## Directory Layout
OCF-HCP-DEVICES
     |
     |_____BloodPressureMonitor
     |
     |
     |_____Iotivity

## File layout inside each simulator

| File                      |  Description                                   |
| --------------------------| ---------------------------------------------- |
| server.cpp                |  Device implementation                         |
| server.idd.dat            |  Device Introspection Device Data (IDD) (cbor) |
| oic_svr_db.dat/server.dat |  Security file (cbor)                          |
| server.idd.json           |  Device Introspection Device Data (IDD) (json) |
| server.dat.json           |  Security file (json)                          |
| PICS/PICS_xxx.json        |  PICS file for CTT tests                       |

