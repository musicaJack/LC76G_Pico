#include "display_manager.hpp"
#ifdef DISPLAY_DRIVER_ILI9488
#include "ili9488_adapter.hpp"
#endif
#include <iostream>

namespace game2048 {

DisplayManager& DisplayManager::getInstance() {
    static DisplayManager instance;
    return instance;
}

bool DisplayManager::initFromFile(const std::string& config_file) {
    // 加载配置
    if (!config_manager_.loadConfig(config_file)) {
        std::cerr << "无法加载配置文件" << std::endl;
        return false;
    }
    
    // 验证配置
    if (!config_manager_.validateConfig()) {
        std::cerr << "配置文件验证失败" << std::endl;
        return false;
    }
    
    // 创建显示驱动
    std::string driver_type = config_manager_.getDriverType();
    driver_ = createDriver(driver_type);
    
    if (!driver_) {
        std::cerr << "无法创建显示驱动: " << driver_type << std::endl;
        return false;
    }
    
    // 应用配置
    if (!applyConfig(driver_.get())) {
        std::cerr << "无法应用配置到显示驱动" << std::endl;
        return false;
    }
    
    // 初始化驱动
    if (!driver_->init()) {
        std::cerr << "显示驱动初始化失败" << std::endl;
        return false;
    }
    
    std::cout << "显示管理器初始化成功，使用驱动: " << driver_type << std::endl;
    return true;
}

bool DisplayManager::init() {
    // 使用嵌入式配置初始化（编译时嵌入的配置）
    std::cout << "使用嵌入式配置初始化显示管理器..." << std::endl;
    
    // 加载嵌入式配置
    if (!config_manager_.loadEmbeddedConfig()) {
        std::cerr << "嵌入式配置加载失败，无法初始化显示管理器" << std::endl;
        return false;
    }
    
    const DisplayConfig& config = config_manager_.getDisplayConfig();
    std::cout << "成功加载嵌入式配置，驱动类型: " << config.driver << std::endl;
    
    // 创建显示驱动
    driver_ = createDriverWithConfig(config);
    
    if (!driver_) {
        std::cerr << "无法创建显示驱动: " << config_manager_.getDriverType() << std::endl;
        return false;
    }
    
    // 初始化显示驱动
    if (!driver_->init()) {
        std::cerr << "显示驱动初始化失败" << std::endl;
        return false;
    }
    
    std::cout << "显示管理器初始化成功（嵌入式配置模式）" << std::endl;
    return true;
}

std::unique_ptr<DisplayDriver> DisplayManager::createDriver(const std::string& driver_type) {
    const DisplayConfig& config = config_manager_.getDisplayConfig();
    return createDriverWithConfig(config);
}

std::unique_ptr<DisplayDriver> DisplayManager::createDriverWithConfig(const DisplayConfig& config) {
    // 使用编译时宏确定要创建的驱动
#ifdef DISPLAY_DRIVER_ST7306
    if (config.driver == "st7306") {
        std::cout << "创建ST7306显示驱动适配器" << std::endl;
        return std::make_unique<ST7306Adapter>(config);
    }
#endif

#ifdef DISPLAY_DRIVER_ILI9488
    if (config.driver == "ili9488") {
        std::cout << "创建ILI9488显示驱动适配器" << std::endl;
        return std::make_unique<ILI9488Adapter>(config);
    }
#endif

    std::cout << "未知的显示驱动类型或未在编译时启用: " << config.driver << std::endl;
    return nullptr;
}

bool DisplayManager::applyConfig(DisplayDriver* driver) {
    if (!driver) {
        return false;
    }
    
    const DisplayConfig& config = config_manager_.getDisplayConfig();
    
    // 设置旋转
    driver->setRotation(config.rotation);
    
    // 设置亮度
    driver->setBrightness(config.brightness);
    
    return true;
}

bool DisplayManager::switchDriver(const std::string& driver_type) {
    if (driver_type == getCurrentDriverType()) {
        return true; // 已经是当前驱动
    }
    
    // 清理当前驱动
    if (driver_) {
        driver_->deinit();
        driver_.reset();
    }
    
    // 创建新驱动
    driver_ = createDriver(driver_type);
    if (!driver_) {
        std::cerr << "无法切换到驱动: " << driver_type << std::endl;
        return false;
    }
    
    // 应用配置
    if (!applyConfig(driver_.get())) {
        std::cerr << "无法应用配置到新驱动" << std::endl;
        return false;
    }
    
    // 初始化新驱动
    if (!driver_->init()) {
        std::cerr << "新驱动初始化失败" << std::endl;
        return false;
    }
    
    // 更新配置
    config_manager_.setDriverType(driver_type);
    
    std::cout << "成功切换到驱动: " << driver_type << std::endl;
    return true;
}

std::string DisplayManager::getCurrentDriverType() const {
    if (!driver_) {
        return "";
    }
    return config_manager_.getDriverType();
}

const DisplayConfig& DisplayManager::getConfig() const {
    return config_manager_.getDisplayConfig();
}

bool DisplayManager::reloadConfig() {
    // 重新加载嵌入式配置
    if (!config_manager_.loadEmbeddedConfig()) {
        return false;
    }
    
    if (!config_manager_.validateConfig()) {
        return false;
    }
    
    // 如果驱动类型改变了，需要切换驱动
    std::string new_driver = config_manager_.getDriverType();
    if (new_driver != getCurrentDriverType()) {
        return switchDriver(new_driver);
    }
    
    // 否则只应用新配置
    return applyConfig(driver_.get());
}

void DisplayManager::cleanup() {
    if (driver_) {
        driver_->deinit();
        driver_.reset();
    }
}

} // namespace game2048
