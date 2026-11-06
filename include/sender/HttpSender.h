#pragma once
#include "ISender.h"
#include <string>

class HttpSender : public ISender {
public:
    // 构造时传入 URL，或者直接根据配置解析
    explicit HttpSender(const std::string& url);
    
    bool send(const std::string& json_payload) override;

private:
    std::string url_;
    std::string host_;
    int port_;
    
    void parseUrl(); // 辅助函数，解析 host 和 port
};
