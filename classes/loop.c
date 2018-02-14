// porres 2017

#include "m_pd.h"

#define OFF      0
#define RUNNING  1
#define PAUSED   2

typedef struct _loop{
    t_object    x_obj;
    t_int       x_target;
    t_int       x_counter_start;
    t_int       x_offset;
    t_int       x_count;
    t_int       x_inc;      
    t_int       x_dir;      // up is "1", down is "-1"
    t_int       x_status;
    t_outlet   *x_bangout;
}t_loop;

static t_class *loop_class;

static void loop_do_loop(t_loop *x){ // The Actual Loop
    x->x_status = RUNNING;
    if(x->x_dir == 1){
        while(x->x_count <= x->x_target){
            outlet_float(((t_object *)x)->ob_outlet, x->x_count + x->x_offset);
            x->x_count = x->x_count + (x->x_dir * x->x_inc);
            if(x->x_status == PAUSED)
                return;
        }
    }
    else{
        while(x->x_count >= x->x_target){
            outlet_float(((t_object *)x)->ob_outlet, x->x_count + x->x_offset);
            x->x_count = x->x_count + (x->x_dir * x->x_inc);
            if(x->x_status == PAUSED)
                return;
        }
    }
    outlet_bang(x->x_bangout);
    x->x_status = OFF;
}

static void loop_bang(t_loop *x){
    if(x->x_status != RUNNING){
        x->x_count = x->x_counter_start; // reset
        loop_do_loop(x);
    }
}

static void loop_float(t_loop *x, t_float f){
    if(f < 1){
        pd_error(x, "loop: number of iterations need to be >= 1");
        return;
    }
    x->x_counter_start = 0;
    x->x_target = (int)f - 1;
    x->x_dir = 1;
    x->x_inc = 1;
    loop_bang(x);
}

static void loop_list(t_loop *x, t_symbol *s, int ac, t_atom *av){
    x->x_counter_start = (int)atom_getfloat(av);
    x->x_target = (int)atom_getfloat(av+1);
    if(ac == 3){
        int inc = (int)atom_getfloat(av+2);
        if(!inc)
            pd_error(x, "[loop]: increment needs to be != 0");
        else
            x->x_inc = inc;
    }
    if(x->x_counter_start > x->x_target)
        x->x_dir = -1;
    else
        x->x_dir = 1;
    loop_bang(x);
}

static void loop_pause(t_loop *x){
    if(x->x_status == RUNNING)
        x->x_status = PAUSED;
}

static void loop_continue(t_loop *x){
    if(x->x_status == PAUSED)
        loop_do_loop(x);
}

static void loop_offset(t_loop *x, t_float f){
    x->x_offset = (int)f;
}

static void loop_inc(t_loop *x, t_float f){
    int inc = (int)f;
    if(!inc)
        pd_error(x, "[loop]: increment needs to be != 0");
    else
        x->x_inc = inc;
}

static void *loop_new(t_symbol *s, int argc, t_atom *argv){
    t_loop *x = (t_loop *)pd_new(loop_class);
    x->x_status = OFF;
    t_float f1 = 0, f2 = 0, offset = 0;
/////////////////////////////////////////////////////////////////////////////////////
    int argnum = 0;
    while(argc > 0){
        if(argv->a_type == A_FLOAT){ // if current argument is a float
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    f1 = argval;
                    break;
                case 1:
                    f2 = argval;
                    break;
                default:
                    break;
            };
            argnum++;
            argc--;
            argv++;
        }
        else if(argv -> a_type == A_SYMBOL){
            t_symbol *curarg = atom_getsymbolarg(0, argc, argv);
            if(!strcmp(curarg->s_name, "-offset")){
                offset = atom_getfloatarg(0, argc, argv+1);
                break;
            }
            else{
                goto errstate;
            };
        }
    };
///////////////////////////////////////////////////////////////////////////////////
    x->x_offset = (int)offset;
    if(f2){
        x->x_counter_start = (int)f1;
        x->x_target = (int)f2;
        if(x->x_counter_start > x->x_target)
            x->x_dir = -1;
        else
            x->x_dir = 1;
    }
    else if(f1){
        if(f1 < 1)
            f1 = 1;
        x->x_target = (int)f1 - 1;
        x->x_dir = 1;
    }
    x->x_inc = 1;
    outlet_new((t_object *)x, &s_float);
    x->x_bangout = outlet_new((t_object *)x, &s_bang);
    return (x);
errstate:
    pd_error(x, "loop: improper args");
    return NULL;
}

void loop_setup(void){
    loop_class = class_new(gensym("loop"), (t_newmethod)loop_new, 0, sizeof(t_loop), 0, A_GIMME, 0);
    class_addbang(loop_class, loop_bang);
    class_addfloat(loop_class, loop_float);
    class_addlist(loop_class, loop_list);
    class_addmethod(loop_class, (t_method)loop_pause, gensym("pause"), 0);
    class_addmethod(loop_class, (t_method)loop_continue, gensym("continue"), 0);
    class_addmethod(loop_class, (t_method)loop_offset, gensym("offset"), A_DEFFLOAT, 0);
    class_addmethod(loop_class, (t_method)loop_inc, gensym("inc"), A_DEFFLOAT, 0);
}
