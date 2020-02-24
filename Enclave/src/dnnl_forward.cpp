#include "dnnl_forward.h"


void dnnl_conv_forward(dnnl_transfer_layer_data *data) {
    engine eng(engine::kind::cpu, 0);
    stream s(eng);
 // data memory descriptor
    auto conv_src_md = memory::desc(
        {data->batch, data->ic, data->h, data->w}, 
        memory::data_type::f32,
        memory::format_tag::any // let convolution choose memory format
    );
    auto conv_weights_md = memory::desc(
        {data->oc, data->ic, data->size, data->size},
        memory::data_type::f32,
        memory::format_tag::any
    );

    auto conv_bias_md = memory::desc(
    {data->oc},
    memory::data_type::f32,
    memory::format_tag::any
    );

    auto conv_dst_md = memory::desc(
        {data->batch, data->oc, data->out_h, data->out_w},
        memory::data_type::f32,
        memory::format_tag::any
    );
    
    // primitive descriptor
    auto conv_pd = convolution_forward::primitive_desc(
        {
            prop_kind::forward_inference,
            algorithm::convolution_auto,
            conv_src_md, conv_weights_md, conv_bias_md, conv_dst_md,
            //  {1, 1}, // strides
            //         {1, 1}, {1, 1}} // left and right padding
            {data->stride, data->stride},
            {data->pad, data->pad}, {data->pad, data->pad}}
            , eng);
    
    // memory object
    auto src_mem = memory(
        {{data->batch, data->ic, data->h, data->w}, memory::data_type::f32,
        memory::format_tag::nchw}, eng);


    auto weights_mem = memory({
        {data->oc, data->ic, data->size, data->size}, memory::data_type::f32,
        memory::format_tag::nchw}, eng);

    auto bias_mem = memory({
        {data->oc}, memory::data_type::f32, memory::format_tag::a}, eng);

    auto dst_mem = memory(
        {{data->batch, data->oc, data->out_h, data->out_w}, memory::data_type::f32, 
        memory::format_tag::nchw},eng);


    write_to_dnnl_memory(data->input, src_mem);
    write_to_dnnl_memory(data->weights, weights_mem);
    write_to_dnnl_memory(data->biases, bias_mem);

    bool need_reorder_src = conv_pd.src_desc() != src_mem.get_desc();
    bool need_reorder_weights = conv_pd.weights_desc() != weights_mem.get_desc();
    bool need_reorder_bias = conv_pd.bias_desc() != bias_mem.get_desc();         
    bool need_reorder_dst = conv_pd.dst_desc() != dst_mem.get_desc();

    auto conv_src_mem = need_reorder_src ? memory(conv_pd.src_desc(), eng) : src_mem;
    auto conv_weights_mem = need_reorder_weights ? memory(conv_pd.weights_desc(), eng) : weights_mem;
    auto conv_bias_mem = need_reorder_bias ? memory(conv_pd.bias_desc(), eng) : bias_mem;
    auto conv_dst_mem = memory(conv_pd.dst_desc(), eng);

    if (need_reorder_src) {
        auto reorder_src = reorder(src_mem, conv_src_mem);
        reorder_src.execute(
                s, {{DNNL_ARG_FROM, src_mem}, {DNNL_ARG_TO, conv_src_mem}});
        s.wait(); // wait for the reorder to complete
    }
    if (need_reorder_weights) {
        auto reorder_weights = reorder(weights_mem, conv_weights_mem);
        reorder_weights.execute(s,
                {{DNNL_ARG_FROM, weights_mem},
                        {DNNL_ARG_TO, conv_weights_mem}});
        s.wait(); // wait for the reorder to complete
    }
    if (need_reorder_bias) {
        auto reorder_bias = reorder(bias_mem, conv_bias_mem);
        reorder_bias.execute(s,
                {{DNNL_ARG_FROM, bias_mem},
                        {DNNL_ARG_TO, conv_bias_mem}});
        s.wait(); // wait for the reorder to complete
    }

    auto conv_scratchpad_mem = memory(conv_pd.scratchpad_desc(), eng);
    auto conv = convolution_forward(conv_pd);
    conv.execute(s,
            {{DNNL_ARG_SRC, conv_src_mem}, {DNNL_ARG_WEIGHTS, conv_weights_mem}, 
            {DNNL_ARG_BIAS, conv_bias_mem}, {DNNL_ARG_DST, conv_dst_mem}});
    s.wait();

    if (need_reorder_dst) {
        auto reorder_dst = reorder(conv_dst_mem, dst_mem);
        reorder_dst.execute(
                s, {{DNNL_ARG_FROM, conv_dst_mem}, {DNNL_ARG_TO, dst_mem}});
        s.wait();
    }
    read_from_dnnl_memory(data->output, dst_mem);
}


void dnnl_conv_forward_aux(dnnl_transfer_layer_data *data) {
    if(handle_errors(dnnl_conv_forward, data) != 0) {
        abort();
    }
}
