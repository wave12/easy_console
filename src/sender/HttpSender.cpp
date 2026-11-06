#include "sender/HttpSender.h"
#include <iostream>
#include <memory>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

HttpSender::HttpSender(const std::string& url) : url_(url), port_(80) {
    parseUrl();
}

void HttpSender::parseUrl() {
    std::string temp = url_;
    if (temp.find("http://") == 0) {
        temp = temp.substr(7);
    } else if (temp.find("https://") == 0) {
        std::cerr << "HTTPS not supported yet." << std::endl;
        return;
    }

    size_t colon_pos = temp.find(':');
    if (colon_pos != std::string::npos) {
        host_ = temp.substr(0, colon_pos);
        port_ = std::stoi(temp.substr(colon_pos + 1));
    } else {
        host_ = temp;
    }
}

bool HttpSender::send(const std::string& json_payload) {
    if (host_.empty()) return false;

    httplib::Result res;
    
    if (host_.rfind("https://", 0) == 0 || port_ == 443) {
        std::string domain = host_;
        if (domain.rfind("https://", 0) == 0) domain = domain.substr(8);
        
        httplib::SSLClient cli(domain, port_);
        cli.set_connection_timeout(3, 0);
        cli.set_read_timeout(5, 0);
        res = cli.Post("/api/metrics", json_payload, "application/json");
    } else {
        std::string domain = host_;
        if (domain.rfind("http://", 0) == 0) domain = domain.substr(7);
        
        httplib::Client cli(domain, port_);
        cli.set_connection_timeout(3, 0);
        cli.set_read_timeout(5, 0);
        res = cli.Post("/api/metrics", json_payload, "application/json");
    }

    if (res && res->status == 200) {
        return true;
    } else {
        std::cerr << "HTTP/HTTPS send failed. Status: " 
                  << (res ? std::to_string(res->status) : "Connection error") << std::endl;
        return false;
    }
}