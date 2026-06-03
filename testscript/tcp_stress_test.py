import socket
import struct
import threading
import time

# ================= 压测参数配置 =================
SERVER_IP = '127.0.0.1'  # 服务器 IP
SERVER_PORT = 8080       # 服务器监听端口 (和 nginx.conf 一致)
CONCURRENCY = 200        # 并发客户端数量（线程数）
REQUESTS_PER_CLIENT = 2000 # 每个客户端循环发送的 Ping 包数量
# ================================================

# 统计数据
successful_requests = 0
failed_requests = 0
total_time = 0

def create_ping_packet():
    """
    构造心跳包。根据 C++ 代码的结构体定义：
    unsigned short pkgLen; (2 bytes)
    unsigned short msgCode; (2 bytes)
    int crc32; (4 bytes)
    总共 8 字节。_CMD_PING 为 0。使用网络字节序(!)。
    """
    pkgLen = 8
    msgCode = 0
    crc32 = 0
    # '!HHi' 代表：网络字节序, unsigned short, unsigned short, int
    return struct.pack('!HHi', pkgLen, msgCode, crc32)

def client_task():
    global successful_requests, failed_requests
    
    ping_packet = create_ping_packet()
    
    try:
        # 创建 TCP Socket 并连接
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # 禁用 Nagle 算法，加速小包发送，避免黏包等待
        client_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1) 
        client_socket.connect((SERVER_IP, SERVER_PORT))
        
        for _ in range(REQUESTS_PER_CLIENT):
            # 发送心跳包
            client_socket.sendall(ping_packet)
            
            # 接收服务器返回的响应（服务器的 Ping 响应也是 8 字节包头）
            response = client_socket.recv(8)
            if len(response) == 8:
                successful_requests += 1
            else:
                failed_requests += 1
                break
                
        client_socket.close()
    except Exception as e:
        failed_requests += REQUESTS_PER_CLIENT

def run_stress_test():
    global total_time
    print(f"[*] 压测目标: {SERVER_IP}:{SERVER_PORT}")
    print(f"[*] 并发连接数: {CONCURRENCY}")
    print(f"[*] 单连接请求数: {REQUESTS_PER_CLIENT}")
    print(f"[*] 总计预估请求: {CONCURRENCY * REQUESTS_PER_CLIENT}")
    print("[*] 正在疯狂发包中，请稍候...")
    
    threads = []
    start_time = time.time()
    
    # 启动所有并发线程
    for _ in range(CONCURRENCY):
        t = threading.Thread(target=client_task)
        threads.append(t)
        t.start()
        
    # 等待所有线程完成
    for t in threads:
        t.join()
        
    end_time = time.time()
    total_time = end_time - start_time
    
    # 打印最终结果
    print("\n================ 压测结果报告 ================")
    print(f"总耗时:      {total_time:.2f} 秒")
    print(f"成功请求:    {successful_requests} 次")
    print(f"失败请求:    {failed_requests} 次")
    if total_time > 0:
        qps = successful_requests / total_time
        print(f"QPS吞吐量:   {qps:.2f} 请求/秒")
    print("==============================================")

if __name__ == '__main__':
    run_stress_test()
