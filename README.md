# MyChat

一个聊天服务器项目，可用工作在 nginx tcp 负载均衡环境中，基于 muduo 库实现的集群聊天服务器和客户端源码。



## 核心技术架构：

- 🚀 **网络通信**：使用 **Muduo** 高性能网络库，基于非阻塞 I/O 和事件驱动模型。
- 🔄 **集群通信**：引入 **Redis** 消息队列（Publish/Subscribe 机制），解决跨服务器消息路由问题。
- 💾 **数据存储**：手写 **MySQL 数据库连接池**，减少 TCP 握手开销，大幅提升数据读写效率。
- ⚖️ **负载均衡**：部署 **Nginx** 进行 TCP 负载均衡，实现服务节点的水平扩展与高可用。
- 💓 **长连接保活**：基于 **TCP 心跳机制** 动态监测客户端在线状态，定时剔除僵尸连接，释放服务器资源。



## 编译

```
# 在项目顶层目录下:
mkdir build && cd build
cmake ..
make
```



## 运行

```
# 服务器端:在项目顶层目下:
cd bin
./ChatServer 127.0.0.1 6000

# 或者

./ChatServer 127.0.0.1 6002
```

```
# 客户端，进入到bin目录下
./ChatClient 127.0.0.1 8000
```



## 架构图

```mermaid
graph TD
    %% 定义样式
    classDef client fill:#e1f5fe,stroke:#01579b,stroke-width:2px;
    classDef nginx fill:#fff9c4,stroke:#fbc02d,stroke-width:2px,shape:hexagon;
    classDef server fill:#e8f5e9,stroke:#2e7d32,stroke-width:2px;
    classDef pool fill:#fff,stroke:#2e7d32,stroke-dasharray: 5 5;
    classDef redis fill:#ffebee,stroke:#c62828,stroke-width:2px;
    classDef db fill:#f3e5f5,stroke:#7b1fa2,stroke-width:2px,shape:cylinder;

    %% --- 1. 客户端接入层 ---
    subgraph ClientLayer [客户端接入层]
        direction LR
        C1(Client A)
        C2(Client B)
        C3(Client N...)
    end

    %% --- 2. 负载均衡层 ---
    LB(Nginx 负载均衡器 <br/>TCP Port: 8000):::nginx

    %% --- 3. 应用服务集群层 ---
    subgraph Cluster [ChatServer 集群]
        direction TB
        
        %% 服务器 1 (详细展示内部结构)
        subgraph ServerNode1 [ChatServer 1]
            direction TB
            CS1[业务逻辑层]:::server
            MP1((MySQL<br/>连接池)):::pool
            CS1 --- MP1
        end

        %% 服务器 2
        subgraph ServerNode2 [ChatServer 2]
            CS2[ChatServer 2]:::server
        end
        
        %% 服务器 N
        subgraph ServerNodeN [ChatServer N]
            CSN[ChatServer N]:::server
        end
    end

    %% --- 4. 中间件与数据层 ---
    Redis[Redis 消息队列<br/>Publish / Subscribe]:::redis
    MySQL[(MySQL 数据库<br/>持久化存储)]:::db

    %% --- 连线逻辑 ---
    
    %% 客户端 -> Nginx
    C1 -- TCP Connection --> LB
    C2 -- TCP Connection --> LB
    C3 -- TCP Connection --> LB

    %% Nginx -> Servers (负载均衡)
    LB -- Route / Balance --> CS1
    LB -- Route / Balance --> CS2
    LB -- Route / Balance --> CSN

    %% Servers -> Redis (发布订阅消息)
    CS1 -- 1.Publish --> Redis
    Redis -.->|2.Subscribe| CS1
    
    CS2 -- Publish --> Redis
    Redis -.->|Subscribe| CS2
    
    CSN -- Publish --> Redis
    Redis -.->|Subscribe| CSN

    %% Servers -> MySQL (数据读写)
    MP1 ==>|SQL Query| MySQL
    CS2 ==>|SQL Query| MySQL
    CSN ==>|SQL Query| MySQL

    %% 应用样式
    class C1,C2,C3 client;
```

