#include "sierrachart.h"

SCDLLName("TradingSystemBasedOnLevelsAndAlertCondition")

#define MAX_CROSSED_LEVELS 100

struct CrossedLevelStore {
    double Levels[MAX_CROSSED_LEVELS];
    int Count = 0;
};

/*==========================================================================*/
SCSFExport scsf_TradingSystemBasedOnLevelsAndAlertCondition(SCStudyInterfaceRef sc)
{
    SCInputRef Input_Enabled = sc.Input[0];
    SCInputRef Input_DeltaThreshold = sc.Input[1];
    SCInputRef Input_ControlBarButtonNumber = sc.Input[2];
    SCInputRef Input_EnableDebuggingOutput = sc.Input[3];
    SCInputRef Input_SourceChart = sc.Input[4];

    SCSubgraphRef Subgraph_BuyEntry = sc.Subgraph[0];
    SCSubgraphRef Subgraph_SellEntry = sc.Subgraph[1];

    if (sc.SetDefaults)
    {
        sc.GraphName = "Trading System Based on Levels and Alert Condition";
        sc.AutoLoop = 1;
        sc.GraphRegion = 0;
        sc.MaintainTradeStatisticsAndTradesData = true;
        sc.MaximumPositionAllowed = 27;
        sc.SupportAttachedOrdersForTrading = true;

        Input_Enabled.Name = "Enabled";
        Input_Enabled.SetYesNo(false);

        Input_DeltaThreshold.Name = "Delta Threshold";
        Input_DeltaThreshold.SetInt(100);

        Input_ControlBarButtonNumber.Name = "ACS Control Bar Button # for Enable/Disable";
        Input_ControlBarButtonNumber.SetInt(1);
        Input_ControlBarButtonNumber.SetIntLimits(1, MAX_ACS_CONTROL_BAR_BUTTONS);

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

    if (sc.MenuEventID == Input_ControlBarButtonNumber.GetInt())
    {
        const int ButtonState = (sc.PointerEventType == SC_ACS_BUTTON_ON) ? 1 : 0;
        Input_Enabled.SetYesNo(ButtonState);
    }

    if (sc.IsFullRecalculation)
    {
        sc.SetCustomStudyControlBarButtonHoverText(Input_ControlBarButtonNumber.GetInt(), "Enable/Disable Trade System");
        sc.SetCustomStudyControlBarButtonEnable(Input_ControlBarButtonNumber.GetInt(), Input_Enabled.GetBoolean());
    }

    if (!Input_Enabled.GetYesNo() || sc.Index == 0)
        return;

    CrossedLevelStore* store = (CrossedLevelStore*)sc.GetPersistentPointer(1);
    if (store == nullptr)
    {
        store = new CrossedLevelStore;
        sc.SetPersistentPointer(1, store);
    }

    // Clear levels at start of session
    if (sc.Index == 1)
    {
        for (int i = 0; i < store->Count; ++i)
            store->Levels[i] = 0.0;
        store->Count = 0;
    }

    auto IsLevelAlreadyCrossed = [&](double level) -> bool {
        for (int i = 0; i < store->Count; ++i)
        {
            if (fabs(store->Levels[i] - level) < 0.01)
                return true;
        }
        return false;
    };

    auto MarkLevelAsCrossed = [&](double level) {
        if (store->Count < MAX_CROSSED_LEVELS)
            store->Levels[store->Count++] = level;
    };

    int SourceChart = Input_SourceChart.GetInt();
    s_UseTool Tool;
    int DrawingIndex = 0;
    double Level = 0.0;
    bool CrossedAnyLevel = false;
    int EvaluatedIndex = sc.Index - 1;

    while (sc.GetUserDrawnChartDrawing(SourceChart, DRAWING_HORIZONTAL_RAY, Tool, DrawingIndex) != 0)
    {
        Level = Tool.BeginValue;
        if (IsLevelAlreadyCrossed(Level)) {
            ++DrawingIndex;
            continue;
        }

        if (sc.High[EvaluatedIndex] > Level && sc.Low[EvaluatedIndex] < Level) {
            MarkLevelAsCrossed(Level);
            CrossedAnyLevel = true;
            break;
        }

        ++DrawingIndex;
    }

    if (!CrossedAnyLevel || sc.Index < 2)
        return;

    int DeltaValue = static_cast<int>(sc.AskVolume[EvaluatedIndex] - sc.BidVolume[EvaluatedIndex]);
    bool IsLongEntry = DeltaValue < Input_DeltaThreshold.GetInt() * -1;
    bool IsShortEntry = DeltaValue > Input_DeltaThreshold.GetInt();

    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);
    bool CanTrade = PositionData.PositionQuantityWithAllWorkingOrders == 0;

    SCString DebugMessage;
    DebugMessage.Format("delta: %d, Level: %.2f, CanTrade: %s, IsLongEntry: %s, IsShortEntry: %s",
        DeltaValue,
        Level,
        CanTrade ? "YES" : "NO",
        IsLongEntry ? "YES" : "NO",
        IsShortEntry ? "YES" : "NO"
    );
    sc.AddMessageToTradeServiceLog(DebugMessage, false);

    if (!CanTrade || (!IsLongEntry && !IsShortEntry))
        return;

    s_SCNewOrder NewOrder;
    NewOrder.OrderQuantity = sc.TradeWindowOrderQuantity;
    NewOrder.OrderType = SCT_ORDERTYPE_MARKET;
    NewOrder.TextTag = "delta imbalance";

    int Result = 0;
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

    if (Input_EnableDebuggingOutput.GetYesNo())
    {
        SCString DebugMessage;
        if (Result >= 0)
        {
            sc.GetTradePosition(PositionData);
            DebugMessage.Format(
                "Index: %d, EvalIndex: %d, Level: %.2f, Delta: %d, Trade: YES, Price: %.2f, Qty: %d",
                sc.Index,
                EvaluatedIndex,
                Level,
                DeltaValue,
                PositionData.AveragePrice,
                PositionData.PositionQuantityWithAllWorkingOrders
            );
            sc.AddMessageToTradeServiceLog(DebugMessage, false);
        }
    }
}
