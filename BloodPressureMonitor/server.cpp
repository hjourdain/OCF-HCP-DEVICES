//******************************************************************
//
// Copyright 2018 Beechwoods Software, inc All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifdef __cplusplus
extern "C" {
#endif

#include "iotivity_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#include <signal.h>
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#include "ocstack.h"
#include "ocpayload.h"

// Comment out to enable the logs
#undef TB_LOG

#include "logger.h"
#define TAG "HCP-BPM-SERVER"

// Platform Info
static const char *gPlatformID = "12341234-1234-1234-1234-123412341234";
static const char *gManufacturerName = "Beechwoods Software, Inc";
static const char *gManufacturerLink = "https://www.beechwoods.com";
static const char *gModelNumber = "BW-BPM-001";
static const char *gDateOfManufacture = "2018-11-24";
static const char *gPlatformVersion = "1.0";
static const char *gOperatingSystemVersion = "Ubuntu 16.04 LTS";
static const char *gHardwareVersion = "DELL PowerEdge T630";
static const char *gFirmwareVersion = "1.0";
static const char *gSupportLink = "https://www.beechwoods.com";
static const char *gSystemTime = "2018-11-24T22:35:00+08:00";
// Configuration files
static const char CRED_FILE[] = "server.dat";
static const char INTROSPECTION_FILE[] = "server.idd.dat";
// Resource URIs
static const char bp_uri[] = "/BP";
static const char pr_uri[] = "/PR";
static const char bpm_am_uri[] = "/BPM-AM";

static OCPlatformInfo platformInfo;

static volatile bool quitFlag = false;
static volatile int bpSystolicValue = 0;
static volatile int bpDiastolicValue = 0;
static volatile int prPulserateValue = 60;

static OCResourceHandle bp;
static OCResourceHandle pr;
static OCResourceHandle bpm_am;
static void (*pnewAmReadyNotificationHandler) (OCResourceHandle resourceHandle) = NULL;
static void (*pAMSendPayloadHandler) (OCResourceHandle amResourceHandle, OCEntityHandlerRequest *ehRequest, OCRepPayload *pResourcePayload);

static FILE* server_fopen(const char *path, const char *mode)
{
    if (0 == strcmp(path, OC_SECURITY_DB_DAT_FILE_NAME))
    {
        return fopen(CRED_FILE, mode);
    }
    else if (0 == strcmp(path, OC_INTROSPECTION_FILE_NAME))
    { 
        return fopen(INTROSPECTION_FILE, mode);
    }
    else
    {
        return fopen(path, mode);
    }
}

#ifdef TB_LOG
static const char *getResult(OCStackResult result) {
    switch (result) {
    case OC_STACK_OK:
        return "OC_STACK_OK";
    case OC_STACK_RESOURCE_CREATED:
        return "OC_STACK_RESOURCE_CREATED";
    case OC_STACK_RESOURCE_DELETED:
        return "OC_STACK_RESOURCE_DELETED";
    case OC_STACK_INVALID_URI:
        return "OC_STACK_INVALID_URI";
    case OC_STACK_INVALID_QUERY:
        return "OC_STACK_INVALID_QUERY";
    case OC_STACK_INVALID_IP:
        return "OC_STACK_INVALID_IP";
    case OC_STACK_INVALID_PORT:
        return "OC_STACK_INVALID_PORT";
    case OC_STACK_INVALID_CALLBACK:
        return "OC_STACK_INVALID_CALLBACK";
    case OC_STACK_INVALID_METHOD:
        return "OC_STACK_INVALID_METHOD";
    case OC_STACK_NO_MEMORY:
        return "OC_STACK_NO_MEMORY";
    case OC_STACK_COMM_ERROR:
        return "OC_STACK_COMM_ERROR";
    case OC_STACK_INVALID_PARAM:
        return "OC_STACK_INVALID_PARAM";
    case OC_STACK_NOTIMPL:
        return "OC_STACK_NOTIMPL";
    case OC_STACK_NO_RESOURCE:
        return "OC_STACK_NO_RESOURCE";
    case OC_STACK_RESOURCE_ERROR:
        return "OC_STACK_RESOURCE_ERROR";
    case OC_STACK_SLOW_RESOURCE:
        return "OC_STACK_SLOW_RESOURCE";
    case OC_STACK_NO_OBSERVERS:
        return "OC_STACK_NO_OBSERVERS";
    case OC_STACK_UNAUTHORIZED_REQ:
        return "OC_STACK_UNAUTHORIZED_REQ";
    #ifdef WITH_PRESENCE
    case OC_STACK_PRESENCE_STOPPED:
        return "OC_STACK_PRESENCE_STOPPED";
    #endif
    case OC_STACK_ERROR:
        return "OC_STACK_ERROR";
    default:
        return "UNKNOWN";
    }
}
#endif

static void *iotivityThread(void *data) {
    struct timespec timeout;

    // Unused parameter data
    (void) data;

    timeout.tv_sec  = 0;
    timeout.tv_nsec = 100000000L;

    OIC_LOG(INFO, TAG, "Entering ocserver main loop...");
    while (!quitFlag)
    {
        if (OCProcess() != OC_STACK_OK)
        {
            OIC_LOG(ERROR, TAG, "OCStack process error");
            return NULL;
        }
        nanosleep(&timeout, NULL);
    }

    OIC_LOG(INFO, TAG, "Exiting ocserver main loop...");

    if (OCStop() != OC_STACK_OK)
    {
        OIC_LOG(ERROR, TAG, "OCStack process error");
    }

    return NULL;
}

static void DeletePlatformInfo()
{
    if (platformInfo.platformID)
        free(platformInfo.platformID);
    if (platformInfo.manufacturerName)
        free(platformInfo.manufacturerName);
    if (platformInfo.manufacturerUrl)
        free(platformInfo.manufacturerUrl);
    if (platformInfo.modelNumber)
        free(platformInfo.modelNumber);
    if (platformInfo.dateOfManufacture)
        free(platformInfo.dateOfManufacture);
    if (platformInfo.platformVersion)
        free(platformInfo.platformVersion);
    if (platformInfo.operatingSystemVersion)
        free(platformInfo.operatingSystemVersion);
    if (platformInfo.hardwareVersion)
        free(platformInfo.hardwareVersion);
    if (platformInfo.firmwareVersion)
        free(platformInfo.firmwareVersion);
    if (platformInfo.supportUrl)
        free(platformInfo.supportUrl);
    if (platformInfo.systemTime)
        free(platformInfo.systemTime);
}

static OCStackResult SetPlatformInfo(const char* platformID, const char *manufacturerName,
    const char *manufacturerUrl, const char *modelNumber, const char *dateOfManufacture,
    const char *platformVersion, const char* operatingSystemVersion, const char* hardwareVersion,
    const char *firmwareVersion, const char* supportUrl, const char* systemTime)
{
    if (manufacturerName != NULL && (strlen(manufacturerName) > MAX_PLATFORM_NAME_LENGTH))
    {
        return OC_STACK_INVALID_PARAM;
    }

    if (manufacturerUrl != NULL && (strlen(manufacturerUrl) > MAX_PLATFORM_URL_LENGTH))
    {
        return OC_STACK_INVALID_PARAM;
    }

    if ((!(platformInfo.platformID = strdup(platformID))) ||
        (!(platformInfo.manufacturerName = strdup(manufacturerName))) ||
        (!(platformInfo.manufacturerUrl = strdup(manufacturerUrl))) ||
        (!(platformInfo.modelNumber = strdup(modelNumber))) ||
        (!(platformInfo.dateOfManufacture = strdup(dateOfManufacture))) ||
        (!(platformInfo.platformVersion = strdup(platformVersion))) ||
        (!(platformInfo.hardwareVersion = strdup(hardwareVersion))) ||
        (!(platformInfo.firmwareVersion = strdup(firmwareVersion))) ||
        (!(platformInfo.operatingSystemVersion = strdup(operatingSystemVersion))) ||
        (!(platformInfo.supportUrl = strdup(supportUrl))) ||
        (!(platformInfo.systemTime = strdup(systemTime))))
    {
        DeletePlatformInfo();
        return OC_STACK_ERROR;
    }
    else
    {
        return OC_STACK_OK;
    }
}

#define VERIFY_SUCCESS(op)                          \
{                                                   \
    if (op !=  OC_STACK_OK)                         \
    {                                               \
        OIC_LOG_V(FATAL, TAG, "%s failed!!", #op);  \
        goto exit;                                  \
    }                                               \
}

static OCStackResult SetDeviceInfo()
{
    OCResourceHandle resourceHandle = OCGetResourceHandleAtUri(OC_RSRVD_DEVICE_URI);
    if (resourceHandle == NULL)
    {
        OIC_LOG(ERROR, TAG, "Device Resource does not exist.");
        goto exit;
    }

    VERIFY_SUCCESS(OCBindResourceTypeToResource(resourceHandle, "oic.d.bloodpressuremonitor"));
    VERIFY_SUCCESS(OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_DEVICE_NAME, "Blood Pressure Monitor"));
    
    VERIFY_SUCCESS(OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_SPEC_VERSION, "ocf.1.1.0"));
    VERIFY_SUCCESS(OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_DATA_MODEL_VERSION,
                                      "ocf.res.1.1.0,ocf.sh.1.1.0"));
    VERIFY_SUCCESS(OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_PROTOCOL_INDEPENDENT_ID,
                                      "12341234-1234-1234-1234-123412341234"));

    OIC_LOG(INFO, TAG, "Device information(Blood Pressure Monitor) initialized successfully.");
    
    return OC_STACK_OK;

exit:
    return OC_STACK_ERROR;
}

static OCEntityHandlerResult BloodPressureEhCb (OCEntityHandlerFlag flag, OCEntityHandlerRequest *ehRequest, void *ehCallbackParam)
{
    OCEntityHandlerResult ret = OC_EH_OK;

    // Unused parameter flag
    (void) flag;
    // Unused parameter ehCallbackParam
    (void) ehCallbackParam;

    OIC_LOG_V(INFO, TAG, "Callback for Blood Pressure resource");

    OCRepPayload* payload = OCRepPayloadCreate();
    if (OC_REST_GET == ehRequest->method)
    {
        OIC_LOG_V(INFO, TAG, "Callback for Blood Pressure resource called with GET method");
        OCRepPayloadSetPropString(payload, "id", "user_example_id");
        OCRepPayloadSetPropInt(payload, "systolic", bpSystolicValue);
        OCRepPayloadSetPropInt(payload, "diastolic", bpDiastolicValue);
        OCRepPayloadSetPropString(payload, "units", "mmHg");

        pAMSendPayloadHandler(bp, ehRequest, payload);
    }
    else
    // Normally impossible, as AM sends only the GET requests!
    {
        OIC_LOG_V (ERROR, TAG, "Received unsupported method %d from client!",
                   ehRequest->method);
        ret = OC_EH_ERROR;
    }

    OCRepPayloadDestroy(payload);

    return ret;
}

static OCEntityHandlerResult PulseRateEhCb (OCEntityHandlerFlag flag, OCEntityHandlerRequest *ehRequest, void *ehCallbackParam)
{
    OCEntityHandlerResult ret = OC_EH_OK;

    // Unused parameter flag
    (void) flag;
    // Unused parameter ehCallbaclParam
    (void) ehCallbackParam;

    OIC_LOG_V(INFO, TAG, "Callback for Pulse Rate resource");

    OCRepPayload* payload = OCRepPayloadCreate();

    if (OC_REST_GET == ehRequest->method)
    {
        OIC_LOG_V(INFO, TAG, "Callback for Pulse Rate resource called with GET method");
        OCRepPayloadSetPropString(payload, "id", "user_example_id");
        OCRepPayloadSetPropInt(payload, "pulserate", prPulserateValue);

        pAMSendPayloadHandler(pr, ehRequest, payload);
    }
    else
    {
        OIC_LOG_V (ERROR, TAG, "Received unsupported method %d from client!",
                ehRequest->method);
        ret = OC_EH_ERROR;
    }

    OCRepPayloadDestroy(payload);

    return ret;
}

#define MAX_COMMAND_SIZE 10
int main()
{
    OCStackResult result;
    char command[MAX_COMMAND_SIZE+1];
    int value, res;
    (void) res;

    OIC_LOG(DEBUG, TAG, "OCServer is starting...");

    // Initialize Persistent Storage for SVR database
    OCPersistentStorage ps = { server_fopen, fread, fwrite, fclose, unlink };
    OCRegisterPersistentStorageHandler(&ps);

    if (OCInit(NULL, 0, OC_SERVER) != OC_STACK_OK)
    {
        OIC_LOG(ERROR, TAG, "OCStack init error");
        return 0;
    }

    result =
    SetPlatformInfo(gPlatformID, gManufacturerName, gManufacturerLink, gModelNumber,
                    gDateOfManufacture, gPlatformVersion, gOperatingSystemVersion,
                    gHardwareVersion, gFirmwareVersion, gSupportLink, gSystemTime);
    if (result != OC_STACK_OK)
    {
        OIC_LOG(INFO, TAG, "Platform info setting failed locally!");
        exit (EXIT_FAILURE);
    }

    result = OCSetPlatformInfo(platformInfo);    
    if (result != OC_STACK_OK)
    {
        OIC_LOG(INFO, TAG, "Platform Registration failed!");
        exit (EXIT_FAILURE);
    } else {
        OIC_LOG(INFO, TAG, "Platform information initialized successfully.");
    }

    result = SetDeviceInfo();
    if (result != OC_STACK_OK)
    {
        OIC_LOG(INFO, TAG, "Device Registration failed!");
        exit (EXIT_FAILURE);
    }

    //Create the resource for monitoring the Blood Pressure
    result = OCCreateResource(&bp,
            "oic.r.blood.pressure",
            OC_RSRVD_INTERFACE_DEFAULT,
            bp_uri,
            BloodPressureEhCb,
            NULL,
            OC_OBSERVABLE | OC_SECURE
        );
    if (result != OC_STACK_OK)
    {
        OIC_LOG_V(INFO, TAG, "Blood Pressure resource creation failed (error = %s)!", getResult(result));
        exit (EXIT_FAILURE);
    } else {
        OIC_LOG(INFO, TAG, "Blood Pressure resource created successfully.");
    }
    result = OCBindResourceInterfaceToResource(bp, OC_RSRVD_INTERFACE_SENSOR);
    if (result != OC_STACK_OK)
    {
        OIC_LOG_V(INFO, TAG, "Binding SENSOR interface to Blood Pressure resource failed (error = %s)!", getResult(result));
        exit (EXIT_FAILURE);
    } else {
        OIC_LOG(INFO, TAG, "SENSOR interface successfully bound to Blood Pressure resource.");
    }

    //Create the resource for monitoring the Pulse Rate
    result = OCCreateResource(&pr,
            "oic.r.pulserate",
            OC_RSRVD_INTERFACE_DEFAULT,
            pr_uri,
            PulseRateEhCb,
            NULL,
            OC_OBSERVABLE | OC_SECURE
        );
    if (result != OC_STACK_OK)
    {
        OIC_LOG_V(INFO, TAG, "Pulse Rate resource creation failed (error = %s)!", getResult(result));
        exit (EXIT_FAILURE);
    } else {
        OIC_LOG(INFO, TAG, "Pulse Rate resource created successfully.");
    }
    result = OCBindResourceInterfaceToResource(pr, OC_RSRVD_INTERFACE_SENSOR);
    if (result != OC_STACK_OK)
    {
        OIC_LOG_V(INFO, TAG, "Binding SENSOR interface to Pulse Rate resource failed (error = %s)!", getResult(result));
        exit (EXIT_FAILURE);
    } else {
        OIC_LOG(INFO, TAG, "SENSOR interface successfully bound to Pulse Rate resource.");
    }

    //Create the resource for the Atomic Measurement
    result = OCCreateResource(&bpm_am,
            "oic.r.bloodpressuremonitor-am",
            OC_RSRVD_INTERFACE_BATCH,       
            bpm_am_uri,
            NULL,
            NULL,
            OC_DISCOVERABLE | OC_OBSERVABLE | OC_SECURE
        );
    if (result != OC_STACK_OK)
    {
        OIC_LOG_V(INFO, TAG, "Atomic measurement resource creation failed (error = %s)!", getResult(result));
        exit (EXIT_FAILURE);
    } else {
        OIC_LOG(INFO, TAG, "Atomic measurement resource created successfully.");
    }
    result = OCBindResourceInterfaceToResource(bpm_am, OC_RSRVD_INTERFACE_LL);
    if (result != OC_STACK_OK)
    {
        OIC_LOG_V(INFO, TAG, "Binding LL interface to atomic measurement resource failed (error = %s)!", getResult(result));
        exit (EXIT_FAILURE);
    } else {
        OIC_LOG(INFO, TAG, "LL interface successfully bound to atomic measurement resource.");
    }
    result = OCBindResourceInterfaceToResource(bpm_am, OC_RSRVD_INTERFACE_DEFAULT);
    if (result != OC_STACK_OK)
    {
        OIC_LOG_V(INFO, TAG, "Binding BASELINE interface to atomic measurement resource failed (error = %s)!", getResult(result));
        exit (EXIT_FAILURE);
    } else {
        OIC_LOG(INFO, TAG, "BASELINE interface successfully bound to atomic measurement resource.");
    }
    result = OCBindResourceTypeToResource(bpm_am, "oic.r.bloodpressuremonitor-am");
    if (result != OC_STACK_OK)
    {
        OIC_LOG_V(INFO, TAG, "Setting resource type of atomic measurement failed (error = %s)!", getResult(result));
        exit (EXIT_FAILURE);
    } else {
        OIC_LOG(INFO, TAG, "Successfully set resource type of atomic measurement.");
    }

    // Add the individual resources to the atomic measurement
    result = OCBindResourceAM(bpm_am, bp, true, &pnewAmReadyNotificationHandler, &pAMSendPayloadHandler);
    if (result != OC_STACK_OK)
    {
        OIC_LOG_V(INFO, TAG, "Adding Blood Pressure resource to the atomic measurement failed (error = %s)!", getResult(result));
        exit (EXIT_FAILURE);
    } else {
        OIC_LOG(INFO, TAG, "Successfully added Blood Pressure resource to the atomic measurement.");
    }

    result = OCBindResourceAM(bpm_am, pr, true, NULL, NULL);
    if (result != OC_STACK_OK)
    {
        OIC_LOG_V(INFO, TAG, "Adding Pulse Rate resource to the atomic measurement failed (error = %s)!", getResult(result));
        exit (EXIT_FAILURE);
    } else {
        OIC_LOG(INFO, TAG, "Successfully added Pulse Rate resource to the atomic measurement.");
    }

    // Add rts-m to the atomic measurement
    result = OCBindRtsMToResource(bpm_am, "oic.r.blood.pressure");
    if (result != OC_STACK_OK)
    {
        OIC_LOG_V(INFO, TAG, "Adding rts-m to the atomic measurement failed (error = %s)!", getResult(result));
        exit (EXIT_FAILURE);
    } else {
        OIC_LOG(INFO, TAG, "Successfully added rts-m to the atomic measurement.");
    }

    int status;
    pthread_t threads[3];
    int thread_id;

    char p2[] = "iotivity_thread";
    thread_id = pthread_create(&threads[0], NULL, iotivityThread, (void *)p2);
    if (thread_id < 0)
    {
        perror("thread create error : ");
        exit(0);
    }

    while (true)
    {
        printf("Please enter a command:\n");
        res = scanf("%10s", command);
        if (strcmp("q", command) == 0)
        {
            break;
        }
        else
        {
            if (strcmp("systolic", command) == 0)
            {
                printf("Enter new systolic value: ");
                res = scanf("%d", &value);
                printf("\n");
                printf("Changing systolic value of Blood Pressure from %d to %d...\n", bpSystolicValue, value);
                bpSystolicValue = value;
                pnewAmReadyNotificationHandler(bpm_am);
            }
            else if (strcmp("diastolic", command) == 0)
            {
                printf("Enter new diastolic value: ");
                res = scanf("%d", &value);
                printf("\n");
                printf("Changing diastolic value of Blood Pressure from %d to %d...\n", bpDiastolicValue, value);
                bpDiastolicValue = value;
                pnewAmReadyNotificationHandler(bpm_am);
            }
            else if (strcmp("pulserate", command) == 0)
            {
                printf("Enter new pulse rate value: ");
                res = scanf("%d", &value);
                printf("\n");
                printf("Changing pulse rate value of Pulse Rate from %d to %d...\n", prPulserateValue, value);
                prPulserateValue = value;
                pnewAmReadyNotificationHandler(bpm_am);
            }
            else if (strcmp("h", command) == 0)
            {
                printf("List of commands: systolic, diastolic, pulserate, h, q\n");
            }
            else
            {
                printf("Unrecognized command (%s)!\n", command);
            }
        }
    }

    printf("Exiting simulator!\n");
    quitFlag = true;
    pthread_join(threads[0], (void **)&status);

    return 0;
}

#ifdef __cplusplus
}
#endif

