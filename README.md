# GoFive

下个版本：

	实现dbsearch
	实现pnsearch(结合ABsearch优化)
    动态深度，解决地平线效应
    优化排序
    内部迭代加深
    多维数组转一维
    改进置换表

v 0.6.1.0

	支持20*20棋盘
	多线程优化(由于gomocup规则，暂不启用)
	添加内存控制
	添加时间分配算法
	优化置换表内存占用
	改进剪枝算法
	完善棋型识别
	优化悔棋、换方逻辑
	添加局面棋型显示模式
	支持长连不胜（standard）规则
	兼容读取psq格式的棋局
	优化调试信息输出方式（回调）

v 0.6.0.0

	[重要]新增宗师级别AI
    去除大师级别AI的一些严重影响效率的策略
    优化提示功能的处理逻辑
    更多AI参数设置
    新的调试信息输出系统
    优化UI的一些展现细节
    添加gomocup支持
    添加64位程序支持

v 0.5.9.0

	重新实现AI底层算法，大幅提升效率
	去除长尾优化

v 0.5.3.0

	调整策略，修复BUG

v 0.5.2.0

	修复一系列新剪枝算法的BUG
	增加搜索范围，增强棋力
	优化剪枝条件，减少计算节点数
	优化长尾算法，能动态更新alphabeta值
	现在内存占用取决于置换表中的数据，可能会超过100M，博弈树建立过程不再消耗额外内存

v 0.5.1.0

	重构代码
	扩大搜索范围，提升AI棋力
	优化棋型匹配算法
	优化置换表，全局锁拆分为层锁
	添加AI计算计时功能
	优化剪枝算法，实现alphabeta剪枝
	彻底解决由于搜索博弈树过于庞大导致的内存占用超过100M的问题
	优化搜索长尾处理，识别到长尾场景后智能增加并发
	根据CPU线程数智能调整搜索深度
	[重要]修复剪枝过程中无法识别禁手的BUG，现在AI会充分利用禁手取胜了

v 0.5.0.1

	添加大师AI，将新算法归入此等级
    进攻算法找到杀着后会保留搜索记录，加速其后的落子
    修复一些新算法的BUG
    优化内存占用

v 0.5.0.0

	重构代码，并减少冗余计算
    [重要]添加进攻搜索算法，大幅加强AI进攻性，略微降低效率
    修复剪枝可能多剪掉的一些分支，增强AI综合棋力，大幅降低效率
    [重要]添加置换表，并借鉴alphabeta优化剪枝算法，大幅提升效率
    此版本AI平均步长较上一版本降低80%
    进攻算法中，同样的结果，步数越多优先级越低

v 0.4.9.1
	
    减少多线程调度优化后的内存占用
    修复一些剪枝BUG，改善了AI的进攻性，避免了重复节点

v 0.4.9.0

    重构代码，改善代码结构和运行效率，分离UI和AI，AI部分基于标准C++11，不再依赖WIN32
	[重要]实现基于字典树的棋型匹配算法，完善棋型模型，棋型识别更为全面准确，效率大幅提升
    优化多线程调度，大幅提升并发效率，大幅提升线程利用率，大幅减少竞争消耗
    AI学会利用禁手获胜
	恢复AI提示功能，优化提示功能展现逻辑
	优化AI等级策略，使等级划分更为合理，降低初级AI难度，初级AI默认无禁手
	修复高DPI下的提示字体显示过大的问题，修复落子时概率闪白屏的问题
	修复一个BUG，导致进攻策略生效时会忽略最优解
    修复一个BUG，随机化算法导致，当有未搜索过的解和最优解相等时，会概率选取未搜索过的解


v 0.4.4.3

    修复进攻搜索时的一个BUG，导致已有杀着时不该冲四却冲四
    添加逻辑防止AI后手开局被布局(AI后手时开局优先防御)

v 0.4.4.2

    修复棋型判定不完整的BUG，如oo?ooo在禁手条件下应等同于xooo而非oo?oo或o?ooo(2015.4.19)
    优化进攻策略，放宽进攻选择条件(2015.4.18)
    修复一些历史遗留问题(修改了一些判定条件)(2015.4.18)
    (感谢陈旋同学的倾力相助)

v 0.4.4.1

    修复进攻时活三误判为死四的BUG(2015.4.18)
    修复进攻时顺水推舟的行为(进攻后把自己送上死路)(2015.4.18)
    (感谢陈旋同学的倾力相助)

v 0.4.4.0

    优化数据结构，减少内存占用(2015.4.16)
    修复一个BUG，导致进攻策略无法在玩家威胁高(10000)的情况下生效(2015.4.17)

v 0.4.3.1

    死四策略进一步优化(2015.4.15)
    大幅调整高级AI策略(2015.4.12)
    添加“显示步数”功能(2015.4.11)
    存档标准化(不再兼容此版本之前的存档，但此版本以后的存档可兼容至高版本)(2015.4.11)

v 0.4.3.0

    暂时禁用“提示”功能(2015.4.10)
    死四策略优化(平时不走，留作绝杀)(简单优化，还有后续)(2015.4.10)
    修复一个bug，导致进攻AI可能会不顾失败的局面(2015.4.10)

v 0.4.2.0

    加强AI进攻性(防守变差)(2015.4.2)
    进一步优化内存占用(优化数据类型)(2015.4.1)
    开局默认禁手开、高级AI(2015.4.1)
    必输局面跟随玩家的落子去堵(防止提示玩家)(2015.4.1)
    进攻得分权重和防守得分权重分离(解决全局威胁权值过大的问题)(2015.2.28)
    修复禁手判定的一个BUG(禁手同时形成5连，禁手依然有效)(2015.2.28)

v 0.4.1.0

    加入多线程支持，界面线程与后台运算线程分离
    禁手系统全局可控化
    多线程优化高级AI计算
    修复玩家后手AI会走禁手的BUG
    修复活三判定（假活三不在内）?ooo? -> ?ooo?? (能形成活四的才是活三)
    高级AI添加随机性

v 0.4.0.0

    添加高级AI（算四步，防御性）
    添加人人对战模式
    优化数据结构，生成博弈树的时候占用内存大幅减小（对于高级AI）
    优化棋型的判定算法
    加入禁手系统（仅高级AI）

v 0.3.2.0

    重新设计数据结构，重写算法
    为高级AI做准备
    代码结构优化，更加面向对象

v 0.3.1.0

    添加伪高级AI(中级AI多算了1步)
    重写算法(kmp)，效率大幅提高
    修复一些AI bug
    修改换方功能，使得对局中随时可以换方而不用重新开局
    添加棋局保存读取功能
    添加AI提示功能

v 0.3.0.0

    重构部分代码，添加中级AI(考虑全局)

v 0.2.9.0

    按照面向对象的思想重构了代码，使扩展更容易（为更新AI做准备）

v 0.2.0.0

    增加换方功能
    修正棋型检测代码
    增加33,34,44检测机制
    修正权重值

v 0.1.0.1

    修复左上方向检测五连的错误
