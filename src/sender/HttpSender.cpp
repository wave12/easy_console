#include "sender/HttpSender.h"
#include <iostream>
#include <memory>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

HttpSender::HttpSender(const std::string& url, const std::string& agent_token) 
    : url_(url), agent_token_(agent_token), port_(80) {
    parseUrl();
}

void HttpSender::parseUrl() {
    std::string temp = url_;
    bool is_https = false;

    // 1. 剥离协议头
    if (temp.rfind("http://", 0) == 0) {
        temp = temp.substr(7);
    } else if (temp.rfind("https://", 0) == 0) {
        temp = temp.substr(8);
        is_https = true;
    }

    // 2. 提取路径 (比如 /reportMetrics)
    size_t slash_pos = temp.find('/');
    if (slash_pos != std::string::npos) {
        path_ = temp.substr(slash_pos); // 保存路径，比如 "/reportMetrics"
        temp = temp.substr(0, slash_pos); // 剩下的就是域名和端口
    } else {
        path_ = "/"; // 默认路径
    }

    // 3. 提取端口
    size_t colon_pos = temp.find(':');
    if (colon_pos != std::string::npos) {
        host_ = temp.substr(0, colon_pos);
        port_ = std::stoi(temp.substr(colon_pos + 1));
    } else {
        host_ = temp;
        port_ = is_https ? 443 : 80; // 默认端口
    }
}

bool HttpSender::send(const std::string& json_payload) {
    std::cout << "[DEBUG] 准备发送数据，当前 Host: " << host_ << " 端口: " << port_ << " 路径: " << path_ << std::endl;
    if (host_.empty()) return false;

    // 准备请求头，把 agent_token 塞进去
    httplib::Headers headers = {
        { "Authorization", "Bearer " + agent_token_ } // 假设你在这个类里加了 agent_token_ 变量
    };

    httplib::Result res;

    // 根据端口或者配置判断是否走 HTTPS
    if (port_ == 443) {
        httplib::SSLClient cli(host_, port_);
        cli.set_connection_timeout(3, 0); 
        cli.set_read_timeout(5, 0);
        cli.enable_server_certificate_verification(false); // 禁用证书验证
        
        // 带上 headers 参数
        res = cli.Post(path_.c_str(), headers, json_payload, "application/json");
    } else {
        httplib::Client cli(host_, port_);
        cli.set_connection_timeout(3, 0); 
        cli.set_read_timeout(5, 0);
        
        // 带上 headers 参数
        res = cli.Post(path_.c_str(), headers, json_payload, "application/json");
    }

    if (res && res->status == 200) {
        try {
            // 尝试解析返回的 JSON 内容
            auto j = nlohmann::json::parse(res->body);
            
            // 如果 JSON 里有 code 并且等于 200，才算是真正的成功
            if (j.contains("code") && j["code"] == 200) {
                std::cout << "[DEBUG] 数据发送成功" << std::endl;
                return true;
            } else {
                // 如果云函数返回了 403 拦截
                std::string msg = j.contains("msg") ? j["msg"].get<std::string>() : "未知业务错误";
                int code = j.contains("code") ? j["code"].get<int>() : 0;
                std::cerr << "[业务拦截] " << msg << " (业务码: " << code << ")" << std::endl;
                return false;
            }
        } catch (const std::exception& e) {
            // 如果解析失败，说明返回的可能不是标准 JSON（比如网关报错）
            std::cerr << "[ERROR] 解析返回 JSON 失败: " << e.what() << std::endl;
            std::cerr << "[原始返回内容] " << res->body << std::endl;
            return false;
        }
    } else {
        std::cerr << "HTTP/HTTPS send failed. Status: ";
        if (res) {
            std::cerr << res->status << std::endl;
            // 打印服务端返回的完整 JSON 内容（核心报错原因）
            std::cerr << "Response body: " << res->body << std::endl;
        } else {
            // 如果 res 是空的，说明根本没连上服务器（比如 DNS 错误、网络不通）
            std::cerr << "Connection error (res is null)" << std::endl;
            // httplib 提供错误码，可以打出来看看是超时还是被拒绝
            std::cerr << "Error code: " << res.error() << std::endl;
        }
        return false;
    }
}