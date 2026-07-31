class CfgPatches
{
    class PIXStore
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"DZ_Data", "DZ_Scripts", "DZ_Characters", "DZ_Vehicles_Wheeled", "JM_CF_Scripts"};
    };
};

class CfgMods
{
    class PIXStore
    {
        dir = "PIXStore";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "PIXStore - Seguro de Veiculos";
        credits = "PackFazupix";
        author = "PackFazupix";
        version = "1.0";
        type = "mod";
        inputs = "PIXStore\inputs\inputs.xml";
        dependencies[] = {"Game","World","Mission"};
        class defs
        {
            class gameScriptModule
            {
                value = "";
                files[] = {"PIXStore/Scripts/3_Game"};
            };
            class worldScriptModule
            {
                value = "";
                files[] = {"PIXStore/Scripts/4_World"};
            };
            class missionScriptModule
            {
                value = "";
                files[] = {"PIXStore/Scripts/5_Mission"};
            };
        };
    };
};
