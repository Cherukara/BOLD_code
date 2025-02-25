function LL = optimPowerScale(xx)
    % Optimization objective function for a scaling factor SR and a power beta.
    % For use within FMINSEARCH (or similar) only.

    global S_dist param1 tau1
    
    loc_param = param1;
    
    loc_param.SR  = xx(1);
    loc_param.sge = xx(2);
%     loc_param.tc_val = xx(3);
%     loc_param.tc_man = 1;
    
    S_model = qASE_model(tau1,param1.TE,loc_param);
    S_model = S_model./max(S_model);
    
    S_mlong = S_model(1:end);
    S_dlong = S_dist(1:end);
    
    LL = log(sum((S_dlong-S_mlong).^2));
    
end
