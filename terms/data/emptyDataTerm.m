%empty term
classdef emptyDataTerm < primalPart & IdentityProxPrimal

    methods
        function obj = emptyDataTerm()
            obj = obj@primalPart(1);
            
            obj.CPPsupport = 1;
        end
    end
end