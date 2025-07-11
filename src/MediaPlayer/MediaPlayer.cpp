#include "MediaPlayer.h"

static bool fullScreenState = false;

int CallbackForTPlayer(void *pUserData, int msg, int param0, void *param1);

MediaPlayer::MediaPlayer(std::string *url)
{
    _prepareFinishFlag = false;

    // 创建播放器
    mTPlayer = TPlayerCreate(CEDARX_PLAYER);
    if (mTPlayer == nullptr)
    {
        printf("[Player] can not create tplayer, quit.\n");
        return;
    }
    TPlayerSetDebugFlag(mTPlayer, false);
    // 设置消息回调函数
    TPlayerSetNotifyCallback(mTPlayer, CallbackForTPlayer, this);

    // 初始化信号量
    sem_init(&_sem, 0, 0);

    // 设置循环播放
    SetLoop(true);

    // url路径不为空，则开始播放音视频
    if (url != nullptr)
    {
        _sourceUrl = *url;
        SetNewVideo(_sourceUrl);
    }
}

MediaPlayer::~MediaPlayer(void)
{
    if (!mTPlayer)
    {
        printf("[Player] player not init.\n");
        return;
    }

    printf("[Player] player reset.\n");
    TPlayerReset(mTPlayer);

    printf("[Player] player destroy.\n");
    TPlayerDestroy(mTPlayer);
    mTPlayer = NULL;

    sem_destroy(&_sem);
}

/**
 * @brief 播放新的视频
 */
bool MediaPlayer::SetNewVideo(std::string &url)
{
    _prepareFinishFlag = false;
    _sourceUrl = url;

    // 复位播放器
    TPlayerReset(mTPlayer);

    // 设置播放文件路径url
    if (TPlayerSetDataSource(mTPlayer, _sourceUrl.c_str(), NULL) != 0)
    {
        printf("[Player] TPlayerSetDataSource() return fail.\n");
        return false;
    }
    else
    {
        printf("[Player] setDataSource end.\n");
    }

    // 解析头部信息
    if (TPlayerPrepareAsync(mTPlayer) != 0)
    {
        printf("[Player] TPlayerPrepareAsync() return fail.\n");
        return false;
    }
    else
    {
        printf("[Player] preparing...\n");
    }

    // 等待信号量，超时时间 3s
    struct timespec timeout = {.tv_sec = 3, .tv_nsec = 0};
    int ret = sem_timedwait(&_sem, &timeout);
    if (ret != 0)
    {
        printf("[Player] MediaPlayer prepare failed, url=%s\n", _sourceUrl.c_str());
        return false;
    }
    else
    {
        printf("[Player] prepared successfully!\n");
    }
    // 不保留最后一帧
    TPlayerSetHoldLastPicture(mTPlayer, 0);
    // 循环播放
    SetLoop(1);

    return true;
}

/**
 * @brief 开始播放
 */
void MediaPlayer::Start(void)
{
    if (_prepareFinishFlag != false)
        TPlayerStart(mTPlayer);
}

/**
 * @brief 暂停播放
 */
void MediaPlayer::Pause(void)
{
    if (_prepareFinishFlag != false)
        TPlayerPause(mTPlayer);
}

/**
 * @brief 设置播放时间点
 * @param seekMs 播放时间点 ms
 */
void MediaPlayer::SetCurrentPos(int seekMs)
{
    if (_prepareFinishFlag != false)
        TPlayerSeekTo(mTPlayer, seekMs);
}

/**
 * @brief 获取当前播放时间点
 * @retval 当前播放的时间点（ms）
 */
int MediaPlayer::GetCurrentPos(void)
{
    int ms = 0;

    if (_prepareFinishFlag != false)
        TPlayerGetCurrentPosition(mTPlayer, &ms);

    return ms;
}

/**
 * @brief 获取播放总时长
 * @retval 播放总时长（ms）
 */
int MediaPlayer::GetDuration(void)
{
    int ms = 3000;

    if (_prepareFinishFlag != false)
        TPlayerGetDuration(mTPlayer, &ms);

    return ms;
}

/**
 * @brief 获取当前音量
 * @retval 当前音量大小（0 ~ 40）
 */
int MediaPlayer::GetVolume(void)
{
    int volume = 0;

    if (_prepareFinishFlag != false)
        volume = TPlayerGetVolume(mTPlayer);

    // printf("[MediaPlayer] getVolume: %d\n", volume);

    return volume;
}

/**
 * @brief 设置当前音量
 * @param volume 设置的音量大小
 */
void MediaPlayer::SetVolume(int volume)
{
    if (_prepareFinishFlag != false)
        TPlayerSetVolume(mTPlayer, volume);

    // printf("[MediaPlayer] setVolume: %d\n", volume);
}

/**
 * @brief 获取当前视频播放状态
 * @retval false 视频未播放
 * @retval true 视频正在播放
 */
bool MediaPlayer::GetState(void)
{
    bool state = false;
    if (_prepareFinishFlag != false)
        state = TPlayerIsPlaying(mTPlayer);

    return state;
}

/**
 * @brief 设置显示的区域
 * @param x: 显示区域起始点 x 坐标的值;
 * @param y: 显示区域起始点 y 坐标的值;
 * @param width: 显示区域的宽;
 * @param height: 显示区域的高.
 * @retval true 成功 / false 失败
 */
bool MediaPlayer::SetDisplayArea(int x, int y, unsigned int width, unsigned int height)
{
    if (_prepareFinishFlag != false)
    {
        TPlayerSetDisplayRect(mTPlayer, x, y, width, height);
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief 设置当前视频播放器是否循环播放
 * @param isLoop 是否循环（true -> 循环 / false -> 不循环）
 */
void MediaPlayer::SetLoop(bool isLoop)
{
    TPlayerSetLooping(mTPlayer, isLoop);
}

/**
 * @brief 设置视频旋转的角度
 * @retval true 成功 / false 失败
 */
bool MediaPlayer::SetRotate(TplayerVideoRotateType rotateDegree)
{
    bool state = false;
    if (TPlayerSetRotate(mTPlayer, rotateDegree) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief 设置视频是否全屏
 * @retval true 成功 / false 失败
 */
bool MediaPlayer::SetFullScreen(bool isFullScreen)
{
    _fullScreenFlag = isFullScreen;

    fullScreenState = isFullScreen;

    printf("[MediaPlayer] SetFullScreen: %d\n", (int)fullScreenState);
    return true;
}

bool MediaPlayer::GetFullScreen(void)
{
    return _fullScreenFlag;
}

/**
 * @brief 设置视频快进快退的速度
 * @retval true 成功 / false 失败
 */
// typedef enum TplayerPlaySpeedType
// {
//     PLAY_SPEED_FAST_FORWARD_16 = 0,  /*fast forward 16 times*/
//     PLAY_SPEED_FAST_FORWARD_8 = 1,   /*fast forward 8 times*/
//     PLAY_SPEED_FAST_FORWARD_4 = 2,   /*fast forward 4 times*/
//     PLAY_SPEED_FAST_FORWARD_2 = 3,   /*fast forward 2 times*/
//     PLAY_SPEED_1              = 4,            /*normal play*/
//     PLAY_SPEED_FAST_BACKWARD_2 = 5,  /*fast backward 2 times*/
//     PLAY_SPEED_FAST_BACKWARD_4 = 6,  /*fast backward  4 times*/
//     PLAY_SPEED_FAST_BACKWARD_8 = 7,  /*fast backward  8 times*/
//     PLAY_SPEED_FAST_BACKWARD_16 = 8, /*fast backward  16 times*/
// } TplayerPlaySpeedType;
bool MediaPlayer::SetSpeed(TplayerPlaySpeedType speed)
{
    bool state = false;
    if (TPlayerSetSpeed(mTPlayer, speed) == 0)
    {
        return true;
    }
    else
    {

        return false;
    }
}

/**
 * @brief  TPlayer消息回调函数
 * @param pUserData 用户数据，设置回调函数时传入
 * @param msg       消息类型
 * @param param0 参赛0
 * @param param1 参数1
 */
int CallbackForTPlayer(void *pUserData, int msg, int param0, void *param1)
{
    MediaPlayer *player = static_cast<MediaPlayer *>(pUserData);

    switch (msg)
    {
    case TPLAYER_NOTIFY_PREPARED:
    {
        // 发送信号量
        printf("[PlayerCb] TPLAYER_NOTIFY_PREPARED\n");
        sem_post(&player->_sem);
        player->_prepareFinishFlag = true;
        break;
    }
    case TPLAYER_NOTIFY_PLAYBACK_COMPLETE:
    {
        printf("[PlayerCb] TPLAYER_NOTIFY_PLAYBACK_COMPLETE\n");
        break;
    }
    case TPLAYER_NOTIFY_SEEK_COMPLETE:
    {
        printf("[PlayerCb] TPLAYER_NOTIFY_SEEK_COMPLETE\n");
        break;
    }
    case TPLAYER_NOTIFY_MEDIA_ERROR:
    {
        switch (param0)
        {
        case TPLAYER_MEDIA_ERROR_UNKNOWN:
        {
            printf("[PlayerCb] erro type: TPLAYER_MEDIA_ERROR_UNKNOWN\n");
            break;
        }
        case TPLAYER_MEDIA_ERROR_UNSUPPORTED:
        {
            printf("[PlayerCb] erro type: TPLAYER_MEDIA_ERROR_UNSUPPORTED\n");
            break;
        }
        case TPLAYER_MEDIA_ERROR_IO:
        {
            printf("[PlayerCb] erro type: TPLAYER_MEDIA_ERROR_IO\n");
            break;
        }
        }
        printf("[PlayerCb] error: open media source fail.\n");
        break;
    }
    case TPLAYER_NOTIFY_NOT_SEEKABLE:
    {
        printf("[PlayerCb] TPLAYER_NOTIFY_NOT_SEEKABLE\n");
        break;
    }
    case TPLAYER_NOTIFY_BUFFER_START:
    {
        printf("[PlayerCb] TPLAYER_NOTIFY_BUFFER_START\n");
        break;
    }
    case TPLAYER_NOTIFY_BUFFER_END:
    {
        printf("[PlayerCb] TPLAYER_NOTIFY_BUFFER_END\n");
        break;
    }
    case TPLAYER_NOTIFY_VIDEO_FRAME:
    {
        /* printf("get the decoded video frame\n"); */
        break;
    }
    case TPLAYER_NOTIFY_AUDIO_FRAME:
    {
        /* printf("get the decoded audio frame\n"); */
        break;
    }
    case TPLAYER_NOTIFY_SUBTITLE_FRAME:
    {
        /* printf("get the decoded subtitle frame\n"); */
        break;
    }
    case TPLAYER_NOTYFY_DECODED_VIDEO_SIZE:
    {
        // TODO: full screen display

        printf("[PlayerCb] ChangeState = %d\n", (int)fullScreenState);

        int w, h, y;
        w = ((int *)param1)[0]; // real decoded video width
        h = ((int *)param1)[1]; // real decoded video height
        printf("[PlayerCb] tplayerdemo: video decoded width = %d, height = %d\n", w, h);

        if (fullScreenState == false)
        {
            float divider = 1;
            if (w > LCD_WIDTH)
            {
                divider = w / LCD_WIDTH;
            }
            w = w / divider;
            h = h / divider;
            y = (LCD_WIDTH - h) / 2;
            printf("[PlayerCb] real set to display rect: w = %d, h = %d\n", w, h);
            player->SetDisplayArea(0, y, w, h);
        }
        else
        {
            player->SetDisplayArea(0, 0, 480, 480);
        }

        break;
    }

    default:
    {
        printf("[PlayerCb] warning: unknown callback from Tinaplayer.\n");
        break;
    }
    }

    return 0;
}
