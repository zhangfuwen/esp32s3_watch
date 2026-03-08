module.exports = {
  apps: [
    {
      name: "opencode-sidecar",
      // 这里的路径根据你的安装方式调整，通常是 opencode-cli 或 opencode serve
      script: "opencode",
      args: "serve --port 19789 --host 0.0.0.0", 
      env: {
        NODE_ENV: "production",
        // 强制开启自动授权模式，防止阻塞
        OPENCODE_AUTO_APPROVE: "true" 
      },
      // 7x24小时关键配置
      restart_delay: 3000,      // 崩溃后延迟3秒重启，防止频繁请求导致封号
      max_restarts: 50,         // 最大连续重试次数
      out_file: "./logs/sidecar-out.log",
      error_file: "./logs/sidecar-err.log",
      merge_logs: true
    }
  ]
};
