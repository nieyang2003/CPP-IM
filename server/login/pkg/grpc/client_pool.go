package pb

import (
	"gate_server/settings"
	sync "sync"

	grpc "google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

var varifyClientPool *GRPCClientPool[VarifyServiceClient]
var routeClientPool *GRPCClientPool[RouteServiceClient]

type GRPCClientPool[T any] struct {
	mu      sync.Mutex
	cond    *sync.Cond
	clients []T
	conns   []*grpc.ClientConn // 用于存储所有的连接，以便在需要时关闭
}

func InitVarifyClientPool(config *settings.VarifyServerConfig) error {
	var err error
	varifyClientPool, err = NewGRPCClientPool(config.Target, config.MaxClients, NewVarifyServiceClient)
	return err
}

func InitRouteClientPool(config *settings.RouteServerConfig) error {
	var err error
	routeClientPool, err = NewGRPCClientPool(config.Target, config.MaxClients, NewRouteServiceClient)
	return err
}

func GetVarifyClientPool() *GRPCClientPool[VarifyServiceClient] {
	return varifyClientPool
}

func GetRouteClientPool() *GRPCClientPool[RouteServiceClient] {
	return routeClientPool
}

func NewGRPCClientPool[T any](target string, maxConns int, newClientFunc func(grpc.ClientConnInterface) T) (*GRPCClientPool[T], error) {
	pool := &GRPCClientPool[T]{
		clients: make([]T, 0, maxConns),
		conns:   make([]*grpc.ClientConn, 0, maxConns),
	}
	pool.cond = sync.NewCond(&pool.mu)

	for i := 0; i < maxConns; i++ {
		conn, err := grpc.Dial(target, grpc.WithTransportCredentials(insecure.NewCredentials()))
		if err != nil {
			return nil, err
		}
		client := newClientFunc(conn)
		pool.clients = append(pool.clients, client)
		pool.conns = append(pool.conns, conn)
	}

	return pool, nil
}

// 安全地取出一个客户端，如果没有可用客户端则阻塞等待
func (pool *GRPCClientPool[T]) Get() T {
	pool.mu.Lock()
	defer pool.mu.Unlock()

	for len(pool.clients) == 0 {
		pool.cond.Wait() // 等待直到有客户端可用
	}

	client := pool.clients[len(pool.clients)-1]
	pool.clients = pool.clients[:len(pool.clients)-1]

	return client
}

// 安全地将客户端放回连接池，并唤醒等待的 goroutine
func (pool *GRPCClientPool[T]) Put(client T) {
	pool.mu.Lock()
	defer pool.mu.Unlock()

	pool.clients = append(pool.clients, client)
	pool.cond.Signal() // 唤醒一个等待的 goroutine
}

// 关闭连接池
func (pool *GRPCClientPool[T]) Close() {
	pool.mu.Lock()
	defer pool.mu.Unlock()

	for _, conn := range pool.conns {
		conn.Close() // 关闭每个连接
	}
	pool.clients = nil // 清空客户端池
	pool.conns = nil   // 清空连接池
}
