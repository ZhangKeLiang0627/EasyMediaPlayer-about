#include "Model.h"

#define VIDEO_DIR "/mnt/UDISK/video/"
#define SD_VIDEO_DIR "/mnt/exUDISK/video/"

using namespace Page;

/* 支持的视频文件格式 */
static const char *fileType[] = {".avi", ".mkv", ".flv", ".ts", ".mp4", ".webm", "asf", "mpg", ".mpeg", ".mov", ".vob", ".3gp", ".wmv", ".pmp"};

/**
 * @brief Model构造函数
 *
 * @param exitCb
 * @param mutex
 */
Model::Model(std::function<void(void)> exitCb, pthread_mutex_t &mutex)
{
    _threadExitFlag = false;
    _mutex = &mutex;

    // 设置UI回调函数
    Operations uiOpts = {0};

    uiOpts.exitCb = exitCb;
    uiOpts.getStateCb = std::bind(&Model::getState, this);
    uiOpts.pauseCb = std::bind(&Model::pause, this);
    uiOpts.playCb = std::bind(&Model::play, this, std::placeholders::_1);
    uiOpts.setCurCb = std::bind(&Model::setCur, this, std::placeholders::_1);
    uiOpts.getCurCb = std::bind(&Model::getCur, this);
    uiOpts.setVolumeCb = std::bind(&Model::setVolume, this, std::placeholders::_1);
    uiOpts.getVolumeCb = std::bind(&Model::getVolume, this);
    uiOpts.getDurationCb = std::bind(&Model::getDuration, this);
    uiOpts.setSpeedCb = std::bind(&Model::setSpeed, this, std::placeholders::_1);
    uiOpts.setRotateCb = std::bind(&Model::setRotate, this, std::placeholders::_1);
    uiOpts.setFullScreenCb = std::bind(&Model::setFullScreen, this, std::placeholders::_1);

     _view.create(uiOpts);

    // 这里设置一个1000ms的定时器，软定时器，用于在onTimerUpdate里update
    _timer = lv_timer_create(onTimerUpdate, 1000, this);

    // 创建执行线程，传递this指针
    pthread_create(&_pthread, NULL, threadProcHandler, this);
}

Model::~Model()
{
    _threadExitFlag = true;

    // 等待线程退出，回收资源
    pthread_join(_pthread, NULL);

    lv_timer_del(_timer);

    _view.release();
}

/**
 * @brief 定时器更新函数
 *
 */
void Model::onTimerUpdate(lv_timer_t *timer)
{
    Model *instance = (Model *)timer->user_data;

    instance->update();
}

/**
 * @brief 更新UI等事务
 *
 */
void Model::update(void)
{
    if (_mp != nullptr)
    {
        // ...
    }
}

/**
 * @brief 线程处理函数
 *
 * @return void*
 */
void *Model::threadProcHandler(void *arg)
{
    Model *model = static_cast<Model *>(arg); // 将arg转换为Model指针

    model->_mp = new MediaPlayer(); // 创建播放器
    // 直接播放某视频
    std::string url = "/mnt/UDISK/video/wallpaper4.mp4";
    model->_mp->SetFullScreen(true);
    model->_mp->SetNewVideo(url);
    model->_mp->Start();

    while (!model->_threadExitFlag)
    {

        usleep(50000);
    }

    delete model->_mp;
}

/**
 * @brief UI获取视频播放状态回调函数
 */
bool Model::getState(void)
{
    bool state = false;

    if (_mp != nullptr)
        state = _mp->GetState();

    return state;
}

/**
 * @brief UI获取音量回调函数
 */
int Model::getVolume(void)
{
    int volume = 0;

    if (_mp != nullptr)
        volume = _mp->GetVolume();

    return volume;
}

/**
 * @brief UI暂停视频回调函数
 */
void Model::pause(void)
{
    if (_mp != nullptr)
        _mp->Pause();
}

/**
 * @brief UI播放视频回调函数
 */
void Model::play(const char *name)
{
    if (name == NULL)
    {
        if (_mp != nullptr)
            _mp->Start(); // 继续播放
        return;
    }

    std::string videoName(name);
    std::string url;

    // TODO：急需一个更加优雅的方法，来分类与加载视频源

    // 检查VIDEO_DIR
    DIR *dir = opendir(VIDEO_DIR);
    if (dir != NULL)
    {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL)
        {
            if (ent->d_type == DT_REG && videoName == ent->d_name)
            {
                url = std::string(VIDEO_DIR) + videoName;
                break;
            }
        }
        closedir(dir);
    }

    // 如果没找到，再检查SD_VIDEO_DIR
    if (url.empty())
    {
        dir = opendir(SD_VIDEO_DIR);
        if (dir != NULL)
        {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL)
            {
                if (ent->d_type == DT_REG && videoName == ent->d_name)
                {
                    url = std::string(SD_VIDEO_DIR) + videoName;
                    break;
                }
            }
            closedir(dir);
        }
    }

    if (!url.empty() && _mp != nullptr)
    {
        _mp->SetNewVideo(url);
        _mp->Start();
    }
}

/**
 * @brief UI设置播放时间点回调函数
 */
void Model::setCur(int cur)
{
    if (_mp != nullptr)
        _mp->SetCurrentPos(cur * 1000);
}

/**
 * @brief UI获取播放时间点回调函数
 */
int Model::getCur(void)
{
    if (_mp != nullptr)
        return _mp->GetCurrentPos(); // 返回单位为ms
    else
        return 0;
}

/**
 * @brief UI获取播放时间点回调函数
 */
int Model::getDuration(void)
{
    if (_mp != nullptr)
        return _mp->GetDuration(); // 返回单位为ms
    else
        return 3000; // 否则返回3000ms，不要为0，不然进度条会出现问题
}

/**
 * @brief UI设置音量回调函数
 */
void Model::setVolume(int volume)
{
    if (_mp != nullptr)
        _mp->SetVolume(volume);
}

/**
 * @brief UI设置倍速回调函数
 */
void Model::setSpeed(int speed)
{
    if (_mp != nullptr)
        _mp->SetSpeed((TplayerPlaySpeedType)speed);
}

/**
 * @brief UI设置翻转屏幕回调函数
 */
void Model::setRotate(int angle)
{
    if (_mp != nullptr)
        _mp->SetRotate((TplayerVideoRotateType)angle);
}

/**
 * @brief UI设置视频是否全屏回调函数
 */
void Model::setFullScreen(bool isFullScreen)
{
    if (_mp != nullptr)
        _mp->SetFullScreen(isFullScreen);
}
