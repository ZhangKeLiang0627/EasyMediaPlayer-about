#include "View.h"
#include <math.h>

// 提前结构体定义，供所有函数使用
typedef struct
{
    lv_obj_t *obj;
    lv_chart_series_t *series_list[3];
} stacked_area_chart_t;

static stacked_area_chart_t stacked_area_chart;

using namespace Page;

#define WAVE_POINT_NUM  40
#define WAVE_SPEED      0.15f
#define WAVE_HEIGHT     30
#define WAVE_BASE       50

static uint32_t wave_tick = 0;

static void wave_timer_cb(lv_timer_t *timer)
{
    stacked_area_chart_t *chart = (stacked_area_chart_t *)timer->user_data;
    lv_chart_series_t *series = chart->series_list[0];
    for (int i = 0; i < WAVE_POINT_NUM; i++)
    {
        float phase = wave_tick * WAVE_SPEED + i * 0.3f;
        int16_t y = WAVE_BASE + (int16_t)(sinf(phase) * WAVE_HEIGHT);
        lv_chart_set_value_by_id(chart->obj, series, i, y);
    }
    lv_chart_refresh(chart->obj);
    wave_tick++;
}

void View::create(Operations &opts)
{
    // 获取View回调函数集
    _opts = opts;

    // 初始化字体
    fontCreate();

    // 总画布的创建
    contCreate(lv_scr_act());

    chartContCreate(ui.cont);

    // topContCreate
    topContCreate(ui.cont);

    // 添加事件回调函数
    AttachEvent(lv_scr_act());
    lv_obj_add_event_cb(ui.topCont.cancelBtn, buttonEventHandler, LV_EVENT_ALL, this);

    /* Transparent background style */
    static lv_style_t style_scr_act;
    if (style_scr_act.prop_cnt == 0)
    {
        lv_style_init(&style_scr_act);
        lv_style_set_bg_opa(&style_scr_act, LV_OPA_COVER);
        lv_obj_add_style(lv_scr_act(), &style_scr_act, 0);
    }

    lv_obj_set_style_bg_opa(ui.cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_img_opa(ui.cont, LV_OPA_TRANSP, 0);

    lv_disp_get_default()
        ->driver->screen_transp = 1;
    lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_TRANSP);
    /* Empty the buffer, not emptying will cause the UI to be opaque */
    lv_memset_00(lv_disp_get_default()->driver->draw_buf->buf_act,
                 lv_disp_get_default()->driver->draw_buf->size * sizeof(lv_color32_t));
    lv_style_set_bg_opa(&style_scr_act, LV_OPA_TRANSP);
    lv_obj_report_style_change(&style_scr_act);

    // 动画的创建
    ui.anim_timeline = lv_anim_timeline_create();
    ui.anim_timelineTop = lv_anim_timeline_create();

#define ANIM_DEF(start_time, obj, attr, start, end) \
    {start_time, obj, LV_ANIM_EXEC(attr), start, end, 500, lv_anim_path_ease_out, true}

#define ANIM_OPA_DEF(start_time, obj) \
    ANIM_DEF(start_time, obj, opa_scale, LV_OPA_COVER, LV_OPA_TRANSP)

    lv_anim_timeline_wrapper_t wrapperTop[] =
        {
            ANIM_DEF(0, ui.topCont.cont, y, -40, lv_obj_get_x_aligned(ui.topCont.cont)),
            ANIM_DEF(0, ui.topCont.cont, width, 20, lv_obj_get_width(ui.topCont.cont)),

            LV_ANIM_TIMELINE_WRAPPER_END // 这个标志着结构体成员结束，不能省略，在下面函数lv_anim_timeline_add_wrapper的轮询中做判断条件
        };
    lv_anim_timeline_add_wrapper(ui.anim_timelineTop, wrapperTop);

    // 开始动画
    appearAnimTop();
}

void View::release()
{
    if (ui.anim_timeline)
    {
        lv_anim_timeline_del(ui.anim_timeline);
        ui.anim_timeline = nullptr;
    }
    if (ui.anim_timelineTop)
    {
        lv_anim_timeline_del(ui.anim_timelineTop);
        ui.anim_timelineTop = nullptr;
    }
    // 移除屏幕手势回调函数
    lv_obj_remove_event_cb(lv_scr_act(), onEvent);
}

void View::appearAnimStart(bool reverse) // 开始开场动画
{
    lv_anim_timeline_set_reverse(ui.anim_timeline, reverse);
    lv_anim_timeline_start(ui.anim_timeline);
}

void View::appearAnimTop(bool reverse) // topCont动画
{
    lv_anim_timeline_set_reverse(ui.anim_timelineTop, reverse);
    lv_anim_timeline_start(ui.anim_timelineTop);

    ui.isTopContCollapsed = reverse;
}

void View::AttachEvent(lv_obj_t *obj)
{
    lv_obj_add_event_cb(obj, onEvent, LV_EVENT_ALL, this);
}

// 自定义字体初始化
void View::fontCreate(void)
{
    ui.fontCont.font16.name = "/mnt/UDISK/font/SmileySans.ttf";
    ui.fontCont.font16.weight = 16;
    ui.fontCont.font16.style = FT_FONT_STYLE_NORMAL;
    ui.fontCont.font16.mem = nullptr;
    lv_ft_font_init(&ui.fontCont.font16);

    ui.fontCont.font20.name = "/mnt/UDISK/font/SmileySans.ttf";
    ui.fontCont.font20.weight = 20;
    ui.fontCont.font20.style = FT_FONT_STYLE_NORMAL;
    ui.fontCont.font20.mem = nullptr;
    lv_ft_font_init(&ui.fontCont.font20);
}

// 总画布的创建
void View::contCreate(lv_obj_t *obj)
{
    lv_obj_t *cont = lv_obj_create(obj);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0xdf9fa4), 0);
    // lv_obj_set_style_bg_img_src(cont, "S:./picture/cover/main1.bin", 0);
    lv_obj_set_style_bg_img_opa(cont, LV_OPA_COVER, 0);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
    ui.cont = cont;

    // 设置图片
    lv_obj_t *img = lv_img_create(obj);
    lv_obj_remove_style_all(img);
    lv_obj_clear_flag(img, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(img, LV_OPA_TRANSP, 0);
    lv_img_set_src(img, "S:./picture/icon/bootlogo.bin");
    lv_obj_align(img, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    ui.image = img;
}

/**
 * Callback which draws the blocks of colour under the lines
 **/
static void draw_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);

    /*Add the faded area before the lines are drawn*/
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
    if (dsc->part == LV_PART_ITEMS)
    {
        if (!dsc->p1 || !dsc->p2)
            return;

        /*Add a line mask that keeps the area below the line*/
        lv_draw_mask_line_param_t line_mask_param;
        lv_draw_mask_line_points_init(&line_mask_param, dsc->p1->x, dsc->p1->y, dsc->p2->x, dsc->p2->y,
                                      LV_DRAW_MASK_LINE_SIDE_BOTTOM);
        int16_t line_mask_id = lv_draw_mask_add(&line_mask_param, NULL);

        /*Draw a rectangle that will be affected by the mask*/
        lv_draw_rect_dsc_t draw_rect_dsc;
        lv_draw_rect_dsc_init(&draw_rect_dsc);
        draw_rect_dsc.bg_opa = LV_OPA_COVER;
        draw_rect_dsc.bg_color = dsc->line_dsc->color;

        lv_area_t a;
        a.x1 = dsc->p1->x;
        a.x2 = dsc->p2->x;
        a.y1 = LV_MIN(dsc->p1->y, dsc->p2->y);
        a.y2 = obj->coords.y2 -
               13; /* -13 cuts off where the rectangle draws over the chart margin. Without this an area of 0 doesn't look like 0 */
        lv_draw_rect(dsc->draw_ctx, &draw_rect_dsc, &a);

        /*Remove the mask*/
        lv_draw_mask_free_param(&line_mask_param);
        lv_draw_mask_remove_id(line_mask_id);
    }
}

/**
 * Helper function to round a fixed point number
 **/
static int32_t round_fixed_point(int32_t n, int8_t shift)
{
    /* Create a bitmask to isolates the decimal part of the fixed point number */
    int32_t mask = 1;
    for (int32_t bit_pos = 0; bit_pos < shift; bit_pos++)
    {
        mask = (mask << 1) + 1;
    }

    int32_t decimal_part = n & mask;

    /* Get 0.5 as fixed point */
    int32_t rounding_boundary = 1 << (shift - 1);

    /* Return either the integer part of n or the integer part + 1 */
    return (decimal_part < rounding_boundary) ? (n & ~mask) : ((n >> shift) + 1) << shift;
}

void View::chartContCreate(lv_obj_t *obj)
{
    stacked_area_chart.obj = lv_chart_create(obj);
    lv_obj_set_size(stacked_area_chart.obj, 200, 150);
    lv_obj_center(stacked_area_chart.obj);
    lv_chart_set_type(stacked_area_chart.obj, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(stacked_area_chart.obj, 0, 0); // 关闭网格线
    lv_obj_add_event_cb(stacked_area_chart.obj, draw_event_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    // 关闭Y轴刻度和标签
    lv_chart_set_axis_tick(stacked_area_chart.obj, LV_CHART_AXIS_PRIMARY_Y, 0, 0, 0, 0, false, 0);
    lv_chart_set_axis_tick(stacked_area_chart.obj, LV_CHART_AXIS_PRIMARY_X, 0, 0, 0, 0, false, 0);
    // 关闭点显示
    lv_obj_set_style_size(stacked_area_chart.obj, 0, LV_PART_INDICATOR);
    // 填充波浪下方区域，颜色加深且不透明
    lv_obj_set_style_opa(stacked_area_chart.obj, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(stacked_area_chart.obj, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(stacked_area_chart.obj, lv_color_hex(0x003366), LV_PART_ITEMS);
    // 只用一条series
    stacked_area_chart.series_list[0] = lv_chart_add_series(stacked_area_chart.obj, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_point_count(stacked_area_chart.obj, WAVE_POINT_NUM);
    lv_timer_create(wave_timer_cb, 30, &stacked_area_chart); // 30ms刷新一次
}

void View::topContCreate(lv_obj_t *obj)
{
    lv_obj_t *cont = lv_obj_create(obj);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, lv_pct(90), lv_pct(8));
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(cont, LV_OPA_90, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0xeeeeee), 0);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_radius(cont, 5, LV_PART_MAIN);
    ui.topCont.cont = cont;

    lv_obj_t *btn = btnCreate(cont, nullptr, 0, 0, 30, 30);
    lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -5, 4);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xff6056), 0);                // 设置按钮默认的颜色
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xe44543), LV_STATE_PRESSED); // 设置按钮在被按下时的颜色
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xe44543), LV_STATE_FOCUSED); // 设置按钮在被按下时的颜色
    ui.topCont.cancelBtn = btn;
    lv_obj_t *cancelBtnLabel = lv_label_create(ui.topCont.cancelBtn);
    lv_obj_remove_style_all(cancelBtnLabel);
    lv_obj_set_style_text_font(cancelBtnLabel, ui.fontCont.font20.font, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cancelBtnLabel, lv_color_hex(0xffffff), 0);
    lv_obj_center(cancelBtnLabel);
    lv_label_set_text_fmt(cancelBtnLabel, "%s", "x");

    btn = btnCreate(cont, nullptr, 0, 0, 40, 30);
    lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -40, 4);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x4ea35a), 0);                // 设置按钮默认的颜色
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x4ea35a), LV_STATE_FOCUSED); // 设置按钮在被聚焦时的颜色 // #ffd76d
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xffd76d), LV_STATE_PRESSED); // 设置按钮在被按下时的颜色
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x646abb), LV_STATE_USER_1);
    ui.topCont.lockBtn = btn;
    lv_obj_t *lockBtnLabel = lv_label_create(ui.topCont.lockBtn);
    lv_obj_remove_style_all(lockBtnLabel);
    lv_obj_set_style_text_font(lockBtnLabel, ui.fontCont.font20.font, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lockBtnLabel, lv_color_hex(0xffffff), 0);
    lv_obj_center(lockBtnLabel);
    lv_label_set_text_fmt(lockBtnLabel, "%s", "锁定");
    ui.topCont.lockLabel = lockBtnLabel;

    btn = btnCreate(cont, nullptr, 0, 0, 40, 30);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 5, 4);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xe09f00), 0);                // 设置按钮默认的颜色
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xe09f00), LV_STATE_FOCUSED); // 设置按钮在被聚焦时的颜色
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xffd76d), LV_STATE_PRESSED); // 设置按钮在被按下时的颜色
    ui.topCont.listBtn = btn;
    lv_obj_t *listBtnLabel = lv_label_create(ui.topCont.listBtn);
    lv_obj_remove_style_all(listBtnLabel);
    lv_obj_set_style_text_font(listBtnLabel, ui.fontCont.font20.font, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(listBtnLabel, lv_color_hex(0xffffff), 0);
    lv_obj_center(listBtnLabel);
    lv_label_set_text_fmt(listBtnLabel, "%s", "列表");

    lv_obj_t *label = lv_label_create(cont);
    lv_obj_remove_style_all(label);
    lv_obj_set_style_text_font(label, ui.fontCont.font20.font, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_black(), 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text_fmt(label, "%s", "about");
}

lv_obj_t *View::btnCreate(lv_obj_t *par, const void *img_src, lv_coord_t x_ofs, lv_coord_t y_ofs, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *obj = lv_obj_create(par);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, w, h);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_align(obj, LV_ALIGN_LEFT_MID, x_ofs, y_ofs);
    lv_obj_set_style_bg_img_src(obj, img_src, 0);

    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_width(obj, w / 1.5f, LV_STATE_PRESSED);                   // 设置button按下时的宽
    lv_obj_set_style_height(obj, h / 1.5f, LV_STATE_PRESSED);                  // 设置button按下时的长
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x356b8c), 0);                 // 设置按钮默认的颜色
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x242947), LV_STATE_PRESSED);  // 设置按钮在被按下时的颜色
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xf2daaa), LV_STATE_FOCUSED);  // 设置按钮在被聚焦时的颜色
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xa99991), LV_STATE_DISABLED); // 设置按钮失能时的颜色
    lv_obj_set_style_radius(obj, 9, 0);                                        // 按钮画圆角

    static lv_style_transition_dsc_t tran;
    static const lv_style_prop_t prop[] = {LV_STYLE_WIDTH, LV_STYLE_HEIGHT, LV_STYLE_PROP_INV};
    lv_style_transition_dsc_init(
        &tran,
        prop,
        lv_anim_path_ease_out,
        150,
        0,
        NULL);
    lv_obj_set_style_transition(obj, &tran, LV_STATE_PRESSED);
    lv_obj_set_style_transition(obj, &tran, LV_STATE_FOCUSED);

    lv_obj_update_layout(obj);

    return obj;
}

void View::buttonEventHandler(lv_event_t *event)
{
    View *instance = (View *)lv_event_get_user_data(event);
    LV_ASSERT_NULL(instance);

    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t *obj = lv_event_get_current_target(event);

    if (code == LV_EVENT_SHORT_CLICKED)
    {

        if (obj == instance->ui.topCont.cancelBtn)
        {
            instance->_opts.exitCb();
        }
    }
}

void View::onEvent(lv_event_t *event)
{
    View *instance = (View *)lv_event_get_user_data(event);
    LV_ASSERT_NULL(instance);

    lv_obj_t *obj = lv_event_get_current_target(event);
    lv_event_code_t code = lv_event_get_code(event);

    if (obj == lv_scr_act())
    {
        if (code == LV_EVENT_GESTURE)
        {
            switch (lv_indev_get_gesture_dir(lv_indev_get_act()))
            {
            case LV_DIR_LEFT:
                printf("[View] LV_DIR_LEFT!\n");

                break;
            case LV_DIR_RIGHT:
                printf("[View] LV_DIR_RIGHT!\n");

                break;
            case LV_DIR_TOP:
                printf("[View] LV_DIR_TOP!\n");
                if (!instance->ui.isTopContCollapsed)
                    instance->appearAnimTop(true);

                break;
            case LV_DIR_BOTTOM:
                printf("[View] LV_DIR_BOTTOM!\n");
                if (instance->ui.isTopContCollapsed)
                    instance->appearAnimTop(false);

                // instance->_opts.exitCb();
                break;

            default:
                break;
            }
        }
    }
}