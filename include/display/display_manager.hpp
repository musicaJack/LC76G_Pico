#pragma once

#include "display_driver.hpp"
#include <memory>
#include <string>

namespace game2048 {

// 显示管理器类 - 工厂模式
class DisplayManager {
public:
    static DisplayManager& getInstance();
    
    // 初始化显示管理器
    bool initFromFile(const std::string& config_file);  // 文件配置版本
    bool init();  // 编译时配置版本（用于嵌入式环境）
    
    // 获取当前显示驱动
    DisplayDriver* getDriver() { return driver_.get(); }
    const DisplayDriver* getDriver() const { return driver_.get(); }
    
    // 切换显示驱动
    bool switchDriver(const std::string& driver_type);
    
    // 获取当前驱动类型
    std::string getCurrentDriverType() const;
    
    // 获取配置
    const DisplayConfig& getConfig() const;
    
    // 重新加载配置
    bool reloadConfig();
    
    // 清理资源
    void cleanup();
    
private:
    DisplayManager() : config_manager_(ConfigManager::getInstance()) {}
    ~DisplayManager() = default;
    DisplayManager(const DisplayManager&) = delete;
    DisplayManager& operator=(const DisplayManager&) = delete;
    
    std::unique_ptr<DisplayDriver> driver_;
    ConfigManager& config_manager_;
    
    // 创建显示驱动实例
    std::unique_ptr<DisplayDriver> createDriver(const std::string& driver_type);
    std::unique_ptr<DisplayDriver> createDriverWithConfig(const DisplayConfig& config);
    
    // 应用配置到驱动
    bool applyConfig(DisplayDriver* driver);
};

} // namespace game2048
