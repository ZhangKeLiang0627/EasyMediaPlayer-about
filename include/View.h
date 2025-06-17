#pragma once

#include "../libs/lvgl/lvgl.h"
#include "../utils/lv_ext/lv_obj_ext_func.h"
#include "../utils/lv_ext/lv_anim_timeline_wrapper.h"
#include <functional>
#include "../utils/smooth_ui_toolkit/src/smooth_ui_toolkit.h"

namespace Page
{
    using ExitCb = std::function<void(void)>;
    using GetPlayStateCb = std::function<bool(void)>;
    using PauseCb = std::function<void(void)>;
    using PlayCb = std::function<void(const char *)>;
    using SetCurCb = std::function<void(int)>;
    using GetCurCb = std::function<int(void)>;
    using GetDurationCb = std::function<int(void)>;
    using GetVolumeCb = std::function<int(void)>;
    using SetVolumeCb = std::function<void(int)>;
    using SetSpeedCb = std::function<void(int)>;
    using SetRotateCb = std::function<void(int)>;
    using SetFullScreenCb = std::function<void(bool)>;

    struct Operations
    {
        ExitCb exitCb;
        GetPlayStateCb getStateCb;       // 获取播放状态
        PauseCb pauseCb;                 // 当前视频暂停回调函数
        PlayCb playCb;                   // 视频播放回调函数
        SetCurCb setCurCb;               // 视频设置进度回调函数
        GetCurCb getCurCb;               // 获取视频进度回调函数
        GetDurationCb getDurationCb;     // 获取视频总长度回调函数
        GetVolumeCb getVolumeCb;         // 获取视频音量回调函数
        SetVolumeCb setVolumeCb;         // 设置视频音量回调函数
        SetSpeedCb setSpeedCb;           // 设置视频倍速回调函数
        SetRotateCb setRotateCb;         // 设置翻转屏幕回调函数
        SetFullScreenCb setFullScreenCb; // 设置视频是否全屏回调函数
    };

    typedef struct
    {
        lv_obj_t *obj;
        lv_chart_series_t *series_list[3];
    } stacked_area_chart_t;

    class View
    {
    private:
        Operations _opts; // View回调函数集
        bool _isPlaying = false;

    public:
        SmoothUIToolKit::Transition3D transition;
        SmoothUIToolKit::Misc::WaterWaveGenerator wave_generator;

        stacked_area_chart_t stacked_area_chart;

        uint32_t currentTime = 0;

        struct
        {
            lv_obj_t *cont;
            lv_obj_t *image;

            struct
            {
                lv_ft_info_t font16; // 自定义字体
                lv_ft_info_t font20; // 自定义字体
            } fontCont;

            struct
            {
                lv_obj_t *cont;
                lv_obj_t *cancelBtn; // to cancel this app
                lv_obj_t *lockBtn;   // to lock the screen
                lv_obj_t *listBtn;   // 播放列表

                lv_obj_t *videoNameLabel; // to show the video name which is playing
                lv_obj_t *lockLabel;      // to show the lock screen label
            } topCont;

            lv_anim_timeline_t *anim_timeline;
            lv_anim_timeline_t *anim_timelineTop;

            bool isTopContCollapsed = false;
        } ui;

        void create(Operations &opts);
        void release(void);
        void appearAnimStart(bool reverse = false);
        void appearAnimTop(bool reverse = false);

    private:
        void AttachEvent(lv_obj_t *obj);

        void contCreate(lv_obj_t *obj);
        void chartContCreate(lv_obj_t *obj);

        void topContCreate(lv_obj_t *obj);
        void fontCreate(void);

        static void onEvent(lv_event_t *event);
        static void buttonEventHandler(lv_event_t *event);

        lv_obj_t *btnCreate(lv_obj_t *par, const void *img_src, lv_coord_t x_ofs, lv_coord_t y_ofs, lv_coord_t w = 50, lv_coord_t h = 50);
    };

}