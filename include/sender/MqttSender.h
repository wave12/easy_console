#pragma once
#include "ISender.h"
#include <string>
#include <mqtt/async_client.h>

class MqttSender : public ISender {
public:
    MqttSender(const std::string& broker, const std::string& client_prefix, 
        const std::string& topic, const std::string& username, 
        const std::string& password);
    ~MqttSender() override;
    bool send(const std::string& json_payload) override;

private:
    std::string topic_;
    mqtt::async_client client_;
    bool connected_;
    std::string generateClientId(const std::string& prefix);
};
