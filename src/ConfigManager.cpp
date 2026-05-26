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

	// JSONデータをインデント付きで書き出す
    file << m_Data.dump(2);

#ifdef _DEBUG
    std::cout << "[ConfigManager] パラメータ保存成功: " << filePath << std::endl;
#endif
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
#ifdef _DEBUG
        std::cout << "[ConfigManager] パラメータ読み込み成功: " << filePath << std::endl;
#endif
    }
    catch (json::parse_error& e)
    {
        std::cerr << "[ConfigManager] JSONパースエラー: " << e.what() << std::endl;
    }
}