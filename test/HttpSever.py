#!/usr/bin/env python3
import json
from http.server import HTTPServer, BaseHTTPRequestHandler

class MonitorReceiver(BaseHTTPRequestHandler):
    def do_POST(self):
        # 获取请求体的长度
        content_length = int(self.headers.get('Content-Length', 0))
        # 读取请求体数据
        post_data = self.rfile.read(content_length).decode('utf-8')
        
        print("\n" + "="*50)
        print(f"Received POST request from {self.client_address[0]}")
        print("-" * 50)
        
        try:
            # 尝试格式化 JSON 输出，方便阅读
            parsed_json = json.loads(post_data)
            print(json.dumps(parsed_json, indent=4, ensure_ascii=False))
        except json.JSONDecodeError:
            # 如果不是合法的 JSON，直接打印原始字符串
            print(post_data)
            
        print("="*50 + "\n")

        # 返回 200 OK 响应
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps({"status": "ok"}).encode('utf-8'))

    def log_message(self, format, *args):
        # 屏蔽默认的控制台日志，只保留我们自己的打印
        pass

def run(server_class=HTTPServer, handler_class=MonitorReceiver, port=5567):
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    print(f"Mock Server started on port {port}...")
    print("Waiting for data from MonitorAgent...\n")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server.")
        httpd.server_close()

if __name__ == '__main__':
    run()