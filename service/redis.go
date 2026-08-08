package main

import (
	"context"
	"github.com/redis/go-redis/v9" // handle Ping(), Set(), Get()
	"encoding/json"
	"os"
	"time"
)

var redisClient *redis.Client // pointer of redis.client
var redisCtx = context.Background() // use for timeout or cancel

func initRedis() error { // go redis is reachable or not and connect it

	//  find redis address from computer/server setting
	addr := os.Getenv("REDIS_ADDR")
	// if not find - assing this address to addr
	if addr == "" { 
		addr = "127.0.0.1:6379"
	}

	// create a client by connection redis server
	redisClient = redis.NewClient(&redis.Options{
		Addr: addr,
		Password: os.Getenv(("REDIS_PASSWORD")), // may be empty
		DB: 0, // use 0th database
	})

	return redisClient.Ping(redisCtx).Err() // if not reachable then generate Err
}

func redisSetString(key string, value string, ttl time.Duration) error {
	return redisClient.Set(redisCtx, key, value, ttl).Err() // store key, value and how much time it will store in redis
}

func redisGetString(key string) (string, error) {
	return redisClient.Get(redisCtx, key).Result()
}

func redisSetJSON(key string, value any, ttl time.Duration) error {
	valueJson, err := json.Marshal(value) // converts go value into json in string into bytes
	if err != nil {
		return err
	}

	return redisClient.Set(redisCtx, key, valueJson, ttl).Err()
}

func redisGetJSON(key string, dest any) error {
	val, err := redisClient.Get(redisCtx, key).Result()
	if err != nil {
		return err
	}

	return json.Unmarshal([]byte(val), dest) // from json in string into bytes to obj
}

