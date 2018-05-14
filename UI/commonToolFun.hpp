#pragma once

#ifndef _COMMON_TOOL_FUN_H_
#define  _COMMON_TOOL_FUN_H_

#include <string>
#include <vector>
#include <QFontMetrics>
#include <QFont>

//#include "core/PrgString.h"
//#include "base/memory/linked_ptr.h"
//#include "base/json/json_helper.h"


//inline PrgString EscapeUrl(PrgString const& url)
//{
//    static const char UnsafeChars[] = ":^&`{}|[]\"<>\\/#?%= @";
//
//    std::string encString = base::WideToUTF8(url.GetString());
//    PrgString Result;
//    for (int i = 0; i < (int)encString.size(); ++i)
//    {
//        if ((encString[i] & 0x80) || strchr(UnsafeChars, encString[i]) != NULL)
//            Result.AppendFormat(L"%%%02x", (uint32)encString[i] & 0xff);
//        else
//            Result += encString[i];
//    }
//    return Result;
//}

/*****************************************
*字符串省略号处理函数，功能将字体为font的字符串
* 处理为当长度大于maxwidth时，大于部分显示为...
*****************************************/
inline QString GetElidedText(QFont font, const QString &str, int MaxWidth)
{
	QString strTemp = str;
	QFontMetrics fontWidth(font);
	int width = fontWidth.width(strTemp);
	if (width >= MaxWidth)	
	{
		strTemp = fontWidth.elidedText(strTemp, Qt::ElideRight, MaxWidth); 
	}
	return strTemp;
}

#endif // !_COMMON_TOOL_FUN_H_
