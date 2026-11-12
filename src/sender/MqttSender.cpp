#include "sender/MqttSender.h"
#include <iostream>
#include <random>
#include <sstream>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#endif

MqttSender::MqttSender(const std::string& broker, const std::string& client_prefix, 
    const std::string& topic, const std::string& username, 
    const std::string& password)
    : topic_(topic), client_(broker, generateClientId(client_prefix)), connected_(false) {

    mqtt::connect_options conn_opts;
    conn_opts.set_keep_alive_interval(20);
    conn_opts.set_clean_session(true);
    conn_opts.set_automatic_reconnect(true);   // 自动重连

    // 核心修改：如果配置了用户名，就设置鉴权
    if (!username.empty()) {
    conn_opts.set_user_name(username);
    conn_opts.set_password(password); 
    }

    try {
    client_.connect(conn_opts)->wait();
    connected_ = true;
    std::cout << "MQTT connected to " << broker << " as " << client_.get_client_id() << std::endl;
    } catch (const mqtt::exception& exc) {
    std::cerr << "MQTT connect failed: " << exc.what() << std::endl;
    }
}


MqttSender::~MqttSender() {
    if (connected_ && client_.is_connected()) {
        try { client_.disconnect()->wait(); } catch (...) {}
    }
}

std::string MqttSender::generateClientId(const std::string& prefix) {
    // 方案：前缀 + 主机名 + 随机数，保证全局唯一且方便排查
    char hostname[256] = {0};
    gethostname(hostname, sizeof(hostname) - 1);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    std::ostringstream oss;
    oss << prefix << hostname << "_" << dis(gen);
    return oss.str();
}

bool MqttSender::send(const std::string& json_payload) {
    if (!client_.is_connected()) {
        try {
            client_.reconnect();
            connected_ = true;
        } catch (const mqtt::exception& exc) {
            std::cerr << "MQTT reconnect failed: " << exc.what() << std::endl;
            return false;
        }
    }

    try {
        nlohmann::json j = nlohmann::json::parse(json_payload);
        //j["agent_token"] = agent_token_; // 假设你在类里存了这个变量
        std::string final_payload = j.dump();
        
        auto msg = mqtt::make_message(topic_, final_payload);
        msg->set_qos(1);
        client_.publish(msg)->wait_for(std::chrono::seconds(5));
        return true;
    } catch (const mqtt::exception& exc) {
        std::cerr << "MQTT publish failed: " << exc.what() << std::endl;
        connected_ = false;
        return false;
    }
}
