% Fabber_Model_Averages.m
%
% Load a Fabber dataset and calculates the average values of the paramaters.
% Designed for use with the phenomenological qBOLD model. Based on the script
% FabberAverages.m

clear; 
% clc;

% Variable names
vars = {'b11','b12','b13','b21','b22','b23','b31','b32','b33'};

% Set number
setnum = 115;

% Slices
slicenum = 1:9;

% Title
disp(['Data from Fabber Set ',num2str(setnum),':']);

% Directory
resdir = '/Users/mattcher/Documents/DPhil/Data/Fabber_ModelFits/';
fdname = dir([resdir,'fabber_',num2str(setnum),'_*']);
fabdir = strcat(resdir,fdname.name,'/');

% See if there is a mean_OEF file in the directory
lst = dir([fabdir,'mean_OEF.nii.gz']);

if ~isempty(lst)
    
    OEFslice = LoadSlice([fabdir,'mean_OEF.nii.gz'],slicenum);
    % DBVslice = LoadSlice([fabdir,'mean_DBV.nii.gz'],slicenum);
    
    % Make a mask of all bad OEF voxels
    badOEF = (OEFslice > 1.0) + (OEFslice < 0.000001);
    
    % calculate average OEFs
    OEFvalues = OEFslice.*(1-badOEF);
    OEFav = sum(OEFvalues,2)./sum(OEFvalues~=0,2);
    
    % vectorize this, for later
    badOEF = badOEF(:);
end


% store all the B values in a single vector (for later analyis)
bb = zeros(length(vars),1);

% Loop through variables
for vv = 1:length(vars)
    
    % Identify Variable
    vname = vars{vv};
    
    % Load Data and Standard Deviation
    Datslice = LoadSlice([fabdir,'mean_',vname,'.nii.gz'],slicenum);
    Stdslice = LoadSlice([fabdir,'std_',vname,'.nii.gz'],slicenum);

    % Vectorize
    Datslice = Datslice(:);
    Stdslice = Stdslice(:);
    
    % Remove bad voxels based on OEF
    if ~isempty(lst)
        Datslice(badOEF ~= 0) = [];
        Stdslice(badOEF ~= 0) = [];
    end
    
    % Remove bad voxels based on ridiculous values (none should be larger than
    % 1e3)
    Stdslice(abs(Datslice) > 1e3) = [];
    Datslice(abs(Datslice) > 1e3) = [];
    
    % Average    
    vmn = nanmean(abs(Datslice));
    vsd = nanmean(abs(Stdslice));
%     vsd = nanstd(abs(Datslice));
    vSR = abs(vmn)./vsd;
    
    % Average and display the results
    disp(['  ',vname,' = ',num2str(vmn),' +/- ',num2str(vsd),' (SNR=',num2str(vSR),')']);
    
    bb(vv) = vmn;

end

%% Free Energy

FEnSlice = LoadSlice([fabdir,'freeEnergy.nii.gz'],slicenum);
ResSlice = LoadSlice([fabdir,'residuals.nii.gz'],slicenum);

% Vectorize
FEnSlice = FEnSlice(:);
ResSlice = ResSlice(:);

% Remove bad voxels
if ~isempty(lst)
    FEnSlice(badOEF ~= 0) = [];
    ResSlice(badOEF ~= 0) = [];
end

% Display
disp('   ');
disp(['Median Free Energy : ',num2str(median(-FEnSlice),4)]);
