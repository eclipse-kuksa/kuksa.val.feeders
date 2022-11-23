/********************************************************************************
* Copyright (c) 2022 Contributors to the Eclipse Foundation
*
* See the NOTICE file(s) distributed with this work for additional
* information regarding copyright ownership.
*
* This program and the accompanying materials are made available under the
* terms of the Apache License 2.0 which is available at
* http://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
********************************************************************************/

#ifndef SOMEIP_CLIENT_H
#define SOMEIP_CLIENT_H

#include <atomic>

#include <vsomeip/vsomeip.hpp>
#include "sample_ids.h"

namespace sdv {
namespace someip {


struct SomeIPRequestConfig {
    bool use_req = false;
    /**
     * @brief SOME/IP Service ID for request/response
     */
    vsomeip::service_t service = SAMPLE_INVALID_VALUE;

    /**
     * @brief SOME/IP Instance ID for request/response
     */
    vsomeip::instance_t instance = SAMPLE_INVALID_VALUE;

    /**
     * @brief SOME/IP Method ID or request/response
     */
    vsomeip::method_t method = SAMPLE_INVALID_VALUE;

    /**
     * @brief SOME/IP Service major version. May be needed if service/someip
     * impl register with major != 0
     */
    vsomeip::major_version_t service_major = vsomeip::ANY_MAJOR;
    /**
     * @brief SOME/IP Service minor version.
     */
    vsomeip::minor_version_t service_minor = vsomeip::ANY_MINOR;
};


/**
 * @brief SOME/IP Client configuration.
 *
 * NOTE: There is a dependency on app_name and config file specified in
 * VSOMEIP_CONFIGURATION environment variable.
 *
 */
struct SomeIPConfig {

    /**
     * @brief vsomeip Application Name. Must match provided app_config json file!
     * Also defined in "VSOMEIP_APPLICATION_NAME" environment variable.
     */
    std::string app_name = "UNKNOWN";

    /**
     * @brief Just a reference to exported "VSOMEIP_CONFIGURATION" environment variable
     */
    std::string app_config = "";

    /**
     * @brief If true, reliable endpoits should be used, depends on the notify server configuration
     */
    bool use_tcp = false;

    /**
     * @brief someip client debug verbosity (0=quiet, ...)
     */
    int debug = 0;

    /**
     * @brief SOME/IP Service ID to subscribe
     */
    vsomeip::service_t service = SAMPLE_SERVICE_ID;

    /**
     * @brief SOME/IP Instance ID to subscribe
     */
    vsomeip::instance_t instance = SAMPLE_INSTANCE_ID;

    /**
     * @brief SOME/IP EventGroup ID
     */
    vsomeip::eventgroup_t event_group = SAMPLE_EVENTGROUP_ID;

    /**
     * @brief SOME/IP Event ID
     */
    vsomeip::event_t event = SAMPLE_EVENT_ID;

    /**
     * @brief SOME/IP Service major version. May be needed if service/someip
     * impl register with major != 0
     */
    vsomeip::major_version_t service_major = vsomeip::ANY_MAJOR;
    /**
     * @brief SOME/IP Service minor version.
     */
    vsomeip::minor_version_t service_minor = vsomeip::ANY_MINOR;

    ///// request response service config
    SomeIPRequestConfig req;
};

/**
 * @brief callback std::function for handling incoming SOME/IP payload
 *
 * @param payload uint8_t* someip notification payload
 * @param size someip payload size
 * @return <0 on error
 */
typedef std::function <
        int (vsomeip::service_t service,
            vsomeip::instance_t instance,
            vsomeip::method_t event,
            const uint8_t *payload,
            size_t size)
    > message_callback_t;

/**
 * @brief Wraps a generic SOME/IP Client for receiving notification events and feeding received raw SOME/IP
 * payload to specified callback for custom decoding
 */
class SomeIPClient
{
public:

    /**
     * @brief Create an Instance of someip client.
     *
     * @param _config SomeIPConfig structure with client configuration
     * @param _callback user provided callback for handling raw event payload
     * @return std::shared_ptr<SomeIPClient>
     */
    static std::shared_ptr<SomeIPClient> createInstance(SomeIPConfig _config, message_callback_t _callback);

    // static SomeIPConfig createDefaultConfig();
    static SomeIPConfig createEnvConfig();

    // SomeIPClient(bool _use_tcp, message_callback_t _callback = nullptr);
    SomeIPClient(SomeIPConfig _config, message_callback_t _callback = nullptr);

    virtual ~SomeIPClient();

    const SomeIPConfig GetConfig() const;

    bool Run();
    void Shutdown();

    int SendRequest(vsomeip::service_t service, vsomeip::instance_t instance, vsomeip::method_t method,
                    std::vector<vsomeip::byte_t> payload);


protected:
    bool init();
    void start(); // blocking call, should be called from a thread
    void stop();

protected:
    SomeIPClient() = default;
    SomeIPClient(const SomeIPClient&) = delete;
    SomeIPClient& operator=(const SomeIPClient&) = delete;

    void on_state(vsomeip::state_type_e _state);
    void on_message(const std::shared_ptr<vsomeip::message> &_response);
    void on_availability(vsomeip::service_t _service, vsomeip::instance_t _instance, bool _is_available);

    void init_event_service(
            vsomeip::service_t service,
            vsomeip::instance_t instance,
            vsomeip::eventgroup_t event_group,
            vsomeip::event_t event,
            vsomeip::major_version_t service_major,
            vsomeip::minor_version_t service_minor);

  protected:
    std::shared_ptr<vsomeip::application> app_;
    std::string name_;
    std::string log_prefix_;
    message_callback_t callback_;
    SomeIPConfig config_;

    std::atomic<bool> stop_requested_;
    std::atomic<bool> initialized_;
    std::mutex stop_mutex_;

    // TODO: use values from config_
    bool use_tcp_;
    vsomeip_v3::service_t       service_;
    vsomeip_v3::major_version_t service_major_;
    vsomeip_v3::minor_version_t service_minor_;
    vsomeip_v3::instance_t      instance_;
    vsomeip_v3::eventgroup_t    event_group_;
    vsomeip_v3::event_t         event_;

    // request service (single)
    bool use_req_;
    bool req_service_available;
    std::mutex req_mutex_;
    std::condition_variable req_condition_;
    // SomeIPRequestConfig req_config_;

    vsomeip_v3::service_t       req_service_;
    vsomeip_v3::major_version_t req_service_major_;
    vsomeip_v3::minor_version_t req_service_minor_;
    vsomeip_v3::instance_t      req_instance_;
    vsomeip_v3::event_t         req_method_;

 };


/**
 * @brief gets string name for vsomeip::message_type_e
 *
 * @param msg_type vsomeip::message_type_e value
 * @return std::string description
 */
std::string message_type_to_string(vsomeip::message_type_e msg_type);

/**
 * @brief
 *
 * @param buf
 * @param size
 * @return std::string
 */
std::string hexdump(uint8_t *buf, size_t size);

/**
 * @brief Get an integer value from Environment variable
 *
 * @param envVar Environment variable name
 * @param defaultValue if envVar was not set
 * @return int result integer
 */
int getEnvironmentInt(const std::string &envVar, int defaultValue);

/**
 * @brief Get std::string value from Environment variable
 *
 * @param envVar Environment variable name
 * @param defaultValue if envVar was not set
 * @return std::string result string
 */
std::string getEnvironmentStr(const std::string &envVar, const std::string &defaultValue);

}  // namespace someip
}  // namespace sdv

#endif // VSOMEIP_WIPER_CLIENT_H
