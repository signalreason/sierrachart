#include "sierrachart.h"

SCDLLName("SwingFailure")

#define MAX_CROSSED_LEVELS 100

struct SwingLevelStore {
    double Levels[MAX_CROSSED_LEVELS];
    COLORREF Colors[MAX_CROSSED_LEVELS];
    bool Traded[MAX_CROSSED_LEVELS];
    int Count = 0;
};

/*==========================================================================*/
SCSFExport scsf_SwingFailure(SCStudyInterfaceRef sc)
{
    SCInputRef Input_Enabled = sc.Input[0];
    SCInputRef Input_ControlBarButtonNumber = sc.Input[1];
    SCInputRef Input_SendOrdersToTradeService = sc.Input[2];
    SCInputRef Input_SendOrdersToTradeServiceButton = sc.Input[3];
    SCInputRef Input_ReloadLevelsButton = sc.Input[4];
    SCInputRef Input_SourceChart = sc.Input[5];
    SCInputRef Input_BuyLevelColor = sc.Input[6];
    SCInputRef Input_SellLevelColor = sc.Input[7];
    SCInputRef Input_OrderType = sc.Input[8];
    SCInputRef Input_EnableDebuggingOutput = sc.Input[9];

    SCSubgraphRef Subgraph_BuyEntry = sc.Subgraph[0];
    SCSubgraphRef Subgraph_SellEntry = sc.Subgraph[1];

    int& LastTradedIndex = sc.GetPersistentInt(3);

    if (sc.SetDefaults)
    {
        sc.GraphName = "Swing Failure";
        sc.AutoLoop = 1;
        sc.GraphRegion = 0;
        sc.MaintainTradeStatisticsAndTradesData = true;
        sc.MaximumPositionAllowed = 27;
        sc.SupportAttachedOrdersForTrading = true;

        Input_Enabled.Name = "Swing Failure Enabled";
        Input_Enabled.SetYesNo(false);

        sc.SendOrdersToTradeService = false;
        Input_SendOrdersToTradeService.Name = "Send Orders to Trade Service";
        Input_SendOrdersToTradeService.SetYesNo(false);

        Input_ControlBarButtonNumber.Name = "ACS Control Bar Button # for Enable/Disable";
        Input_ControlBarButtonNumber.SetInt(1);
        Input_ControlBarButtonNumber.SetIntLimits(1, MAX_ACS_CONTROL_BAR_BUTTONS);

        Input_SendOrdersToTradeServiceButton.Name = "Button # to toggle Send Orders to Trade Service";
        Input_SendOrdersToTradeServiceButton.SetInt(2);
        Input_SendOrdersToTradeServiceButton.SetIntLimits(1, MAX_ACS_CONTROL_BAR_BUTTONS);

        // input to choose the color for buy levels
        Input_BuyLevelColor.Name = "Buy Level Color";
        Input_BuyLevelColor.SetColor(RGB(0, 255, 0));

        // input to choose the color for sell levels
        Input_SellLevelColor.Name = "Sell Level Color";
        Input_SellLevelColor.SetColor(RGB(255, 0, 0));

        // input to choose the order type for entries
        Input_OrderType.Name = "Order Type for Entries";
        Input_OrderType.SetCustomInputStrings("Market;Limit");
        Input_OrderType.SetCustomInputIndex(0);  // Default to Market order

        Input_ReloadLevelsButton.Name = "Button # to reload levels from source chart";
        Input_ReloadLevelsButton.SetInt(3);
        Input_ReloadLevelsButton.SetIntLimits(1, MAX_ACS_CONTROL_BAR_BUTTONS);

        Input_EnableDebuggingOutput.Name = "Enable Debugging Output";
        Input_EnableDebuggingOutput.SetYesNo(true);

        Input_SourceChart.Name = "Source Chart Number for Levels";
        Input_SourceChart.SetInt(9);
        Input_SourceChart.SetIntLimits(1, 100);

        Subgraph_BuyEntry.Name = "Buy";
        Subgraph_BuyEntry.DrawStyle = DRAWSTYLE_POINT_ON_LOW;
        Subgraph_BuyEntry.LineWidth = 5;

        Subgraph_SellEntry.Name = "Sell";
        Subgraph_SellEntry.DrawStyle = DRAWSTYLE_POINT_ON_HIGH;
        Subgraph_SellEntry.LineWidth = 5;

        return;
    }

    if (sc.IsFullRecalculation)
    {
        sc.SetCustomStudyControlBarButtonHoverText(Input_ControlBarButtonNumber.GetInt(), "Enable/Disable Swing Failure");
        sc.SetCustomStudyControlBarButtonEnable(Input_ControlBarButtonNumber.GetInt(), Input_Enabled.GetBoolean());
        sc.SetCustomStudyControlBarButtonHoverText(Input_SendOrdersToTradeServiceButton.GetInt(), "Enable/Disable Send Orders to Trade Service");
        sc.SetCustomStudyControlBarButtonEnable(Input_SendOrdersToTradeServiceButton.GetInt(), Input_SendOrdersToTradeService.GetBoolean());
        sc.SetCustomStudyControlBarButtonHoverText(Input_ReloadLevelsButton.GetInt(), "Reload Levels from Source Chart");
        sc.SetCustomStudyControlBarButtonEnable(Input_ReloadLevelsButton.GetInt(), false);
        LastTradedIndex = 1;
    }

    if (sc.MenuEventID == Input_ControlBarButtonNumber.GetInt())
    {
        const int ButtonState = (sc.PointerEventType == SC_ACS_BUTTON_ON) ? 1 : 0;
        Input_Enabled.SetYesNo(ButtonState);
        SCString Message;
        Message.Format("Swing Failure is now %s", ButtonState ? "Enabled" : "Disabled");
        sc.AddMessageToTradeServiceLog(Message, false);
    }

    if (sc.MenuEventID == Input_SendOrdersToTradeServiceButton.GetInt())
    {
        const int ButtonState = (sc.PointerEventType == SC_ACS_BUTTON_ON) ? 1 : 0;
        sc.SendOrdersToTradeService = ButtonState;
        Input_SendOrdersToTradeService.SetYesNo(ButtonState);
        SCString Message;
        Message.Format("Send Orders to Trade Service is now %s", ButtonState ? "Enabled" : "Disabled");
        sc.AddMessageToTradeServiceLog(Message, false);
    }

    if (!Input_Enabled.GetYesNo() || sc.Index == 0)
        return;

    SwingLevelStore* swingLevels = (SwingLevelStore*)sc.GetPersistentPointer(2);
    if (swingLevels == nullptr)
    {
        swingLevels = new SwingLevelStore;
        sc.SetPersistentPointer(2, swingLevels);
    }

    auto LoadDrawings = [&](int chartNumber) {
        s_UseTool Tool;
        int DrawingIndex = 0;
        swingLevels->Count = 0;
        while (sc.GetUserDrawnChartDrawing(chartNumber, DRAWING_HORIZONTAL_RAY, Tool, DrawingIndex) != 0)
        {
            // Only consider levels with color (ignore colorless)
            if (Tool.Color == Input_BuyLevelColor.GetColor() || Tool.Color == Input_SellLevelColor.GetColor()) {
                swingLevels->Levels[swingLevels->Count] = Tool.BeginValue;
                swingLevels->Colors[swingLevels->Count] = Tool.Color;
                swingLevels->Traded[swingLevels->Count] = false;
                swingLevels->Count++;
                if (swingLevels->Count >= MAX_CROSSED_LEVELS)
                    break;
                // Log each level found
                SCString msg;
                msg.Format("Loaded level %.2f, color: 0x%06X", Tool.BeginValue, Tool.Color);
                sc.AddMessageToTradeServiceLog(msg, false);
            }
            ++DrawingIndex;
        }
        if (swingLevels->Count == 0)
        {
            SCString NoLevelsMessage;
            NoLevelsMessage.Format("No colored levels found in source chart %d. Exiting processing.", chartNumber);
            sc.AddMessageToTradeServiceLog(NoLevelsMessage, false);
            return;
        }
    };

    auto ClearLevels = [&]() {
        for (int i = 0; i < swingLevels->Count; ++i) {
            swingLevels->Levels[i] = 0.0;
            swingLevels->Traded[i] = false;
        }
        swingLevels->Count = 0;
    };

    auto ReloadLevels = [&]() {
        int SourceChart = Input_SourceChart.GetInt();
        ClearLevels();
        LoadDrawings(SourceChart);
    };

    if (sc.MenuEventID == Input_ReloadLevelsButton.GetInt())
    {
        ReloadLevels();
        SCString ReloadMessage;
        ReloadMessage.Format("Reloaded levels from source chart %d. Found %d colored levels.", Input_SourceChart.GetInt(), swingLevels->Count);
        sc.AddMessageToTradeServiceLog(ReloadMessage, false);
        sc.SetCustomStudyControlBarButtonEnable(Input_ReloadLevelsButton.GetInt(), false);
    }

    s_UseTool Tool;
    int DrawingIndex = 0;

    // Clear levels at start of session
    if (sc.Index == 1)
    {
        ReloadLevels();
    }

    double Level = 0.0;
    COLORREF LevelColor = 0;
    int TradedLevelIndex = -1;
    int EvaluatedIndex = sc.Index - 1;
    SCString DebugMessage;
    bool IsLongEntry = false;
    bool IsShortEntry = false;
    double SignalHigh = 0.0;
    double SignalLow = 0.0;
    double SignalClose = 0.0;

    for (int i = 0; i < swingLevels->Count; ++i) {
        if (swingLevels->Traded[i])
            continue;
        Level = swingLevels->Levels[i];
        LevelColor = swingLevels->Colors[i];
        // Green = long only, Red = short only
        // LBAF (look-below-and-fail): low < level, close > level (long, only for green)
        // LAAF (look-above-and-fail): high > level, close < level (short, only for red)
        double high = sc.High[EvaluatedIndex];
        double low = sc.Low[EvaluatedIndex];
        double close = sc.Close[EvaluatedIndex];
        if (LevelColor == RGB(0,255,0)) { // green
            if (low < Level && close > Level) {
                IsLongEntry = true;
                SignalLow = low;
                SignalClose = close;
                TradedLevelIndex = i;
                break;
            }
        } else if (LevelColor == RGB(255,0,0)) { // red
            if (high > Level && close < Level) {
                IsShortEntry = true;
                SignalHigh = high;
                SignalClose = close;
                TradedLevelIndex = i;
                break;
            }
        }
    }

    if ((!IsLongEntry && !IsShortEntry) || sc.Index < 2)
        return;

    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);
    bool CanTrade = PositionData.PositionQuantityWithAllWorkingOrders == 0;

    if (!CanTrade)
        return;

    s_SCNewOrder NewOrder;
    NewOrder.OrderQuantity = sc.TradeWindowOrderQuantity;
    NewOrder.OrderType = SCT_ORDERTYPE_MARKET;
    NewOrder.TextTag = "swing failure";

    if (Input_OrderType.GetSelectedCustomString() == "Limit") // Limit order
    {
        NewOrder.OrderType = SCT_ORDERTYPE_LIMIT;
        NewOrder.Price1 = Level;
    }

    int Result = 0;

    if (TradedLevelIndex != -1 && LastTradedIndex < EvaluatedIndex)
    {
        if (IsLongEntry)
        {
            Result = sc.BuyEntry(NewOrder);
            Subgraph_BuyEntry[EvaluatedIndex] = 1;
            Subgraph_SellEntry[EvaluatedIndex] = 0;
        }
        else if (IsShortEntry)
        {
            Result = sc.SellEntry(NewOrder);
            Subgraph_BuyEntry[EvaluatedIndex] = 0;
            Subgraph_SellEntry[EvaluatedIndex] = 1;
        }
        LastTradedIndex = EvaluatedIndex;
        swingLevels->Traded[TradedLevelIndex] = true;
    }

    if (Input_EnableDebuggingOutput.GetYesNo() && Result < 0)
    {
        SCString ErrorMessage;
        ErrorMessage.Format("Trade failed at index %d: %s", sc.Index, sc.GetTradingErrorTextMessage(Result));
        sc.AddMessageToTradeServiceLog(ErrorMessage, false);
        return;
    }

    if (Input_EnableDebuggingOutput.GetYesNo() && Result >= 0)
    {
        SCString DebugMessage;
        sc.GetTradePosition(PositionData);
        if (IsLongEntry) {
            DebugMessage.Format(
                "Index: %d, EvalIndex: %d, Level: %.2f, LBAF, Low: %.2f, Close: %.2f, Trade: YES, Price: %.2f, Qty: %d",
                sc.Index,
                EvaluatedIndex,
                Level,
                SignalLow,
                SignalClose,
                PositionData.AveragePrice,
                PositionData.PositionQuantityWithAllWorkingOrders
            );
        } else if (IsShortEntry) {
            DebugMessage.Format(
                "Index: %d, EvalIndex: %d, Level: %.2f, LAAF, High: %.2f, Close: %.2f, Trade: YES, Price: %.2f, Qty: %d",
                sc.Index,
                EvaluatedIndex,
                Level,
                SignalHigh,
                SignalClose,
                PositionData.AveragePrice,
                PositionData.PositionQuantityWithAllWorkingOrders
            );
        }
        sc.AddMessageToTradeServiceLog(DebugMessage, false);
    }
}
