#pragma once
#include "json.hpp"


using json = nlohmann::json;

class ConfigManager
{
public:

	// シングルトンインスタンス取得
    static ConfigManager& GetInstance()
    {
        static ConfigManager instance;
        return instance;
    }

    // JSONデータの読み込み
    void Load(const std::string& filePath);

	// JSONデータの取得
    const json& GetData() const { return m_Data; }

private:

    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    json m_Data;
};