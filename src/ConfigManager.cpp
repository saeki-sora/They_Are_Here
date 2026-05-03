#include "pch.h"
#include "ConfigManager.h"

void ConfigManager::Save(const std::string& filePath)
{
    std::ofstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "[ConfigManager] Error: 書き込めません: " << filePath << std::endl;
        return;
    }
    // インデント幅2でdump（読みやすい整形済みJSON）
    file << m_Data.dump(2);
    std::cout << "[ConfigManager] パラメータ保存成功: " << filePath << std::endl;
}

void ConfigManager::Load(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "[ConfigManager] Error: ファイルが開けません: " << filePath << std::endl;
        return;
    }

    try
    {
        file >> m_Data;
        std::cout << "[ConfigManager] パラメータ読み込み成功: " << filePath << std::endl;
    }
    catch (json::parse_error& e)
    {
        std::cerr << "[ConfigManager] JSONパースエラー: " << e.what() << std::endl;
    }
}