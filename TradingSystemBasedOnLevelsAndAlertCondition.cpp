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
	SCInputRef Input_NumBarsToCalculate = sc.Input[1];

	SCInputRef Input_DrawStyleOffsetType = sc.Input[2];
	SCInputRef Input_PercentageOrTicksOffset = sc.Input[3];

	SCInputRef Input_EvaluateOnBarCloseOnly = sc.Input[4];
	SCInputRef Input_SendTradeOrdersToTradeService = sc.Input[5];
	SCInputRef Input_MaximumPositionAllowed = sc.Input[6];
	SCInputRef Input_MaximumLongTradesPerDay = sc.Input[7];
	SCInputRef Input_MaximumShortTradesPerDay = sc.Input[8];
	SCInputRef Input_EnableDebuggingOutput = sc.Input[9];
	SCInputRef Input_CancelAllWorkingOrdersOnExit = sc.Input[10];

	SCInputRef Input_AllowTradingOnlyDuringTimeRange = sc.Input[11];
	SCInputRef Input_StartTimeForAllowedTimeRange = sc.Input[12];
	SCInputRef Input_EndTimeForAllowedTimeRange = sc.Input[13];
	SCInputRef Input_AllowOnlyOneTradePerBar = sc.Input[14];
	SCInputRef Input_Version = sc.Input[15];
	SCInputRef Input_SupportReversals = sc.Input[16];
	SCInputRef Input_AllowMultipleEntriesInSameDirection = sc.Input[17];
	SCInputRef Input_AllowEntryWithWorkingOrders = sc.Input[18];
	SCInputRef Input_ControlBarButtonNumberForEnableDisable = sc.Input[19];
	SCInputRef Input_CancelAllOrdersOnEntries = sc.Input[20];
	SCInputRef Input_CancelAllOrdersOnReversals = sc.Input[21];
	SCInputRef Input_OrderTextTag = sc.Input[22];
    SCInputRef Input_DeltaStudyRef = sc.Input[23];
    SCInputRef Input_DeltaThreshold = sc.Input[24];

	SCSubgraphRef Subgraph_BuyEntry = sc.Subgraph[0];
	SCSubgraphRef Subgraph_SellEntry = sc.Subgraph[1];

	if (sc.SetDefaults)
	{
		// Set the configuration and defaults

		sc.GraphName = "Trading System Based on Levels and Alert Condition";

		sc.StudyDescription = "";

		sc.AutoLoop = 0;  // manual looping
		sc.GraphRegion = 0;
		sc.CalculationPrecedence = LOW_PREC_LEVEL;

		Input_Enabled.Name = "Enabled";
		Input_Enabled.SetYesNo(false);

		Input_NumBarsToCalculate.Name = "Number of Bars to Calculate";
		Input_NumBarsToCalculate.SetInt(2000);
		Input_NumBarsToCalculate.SetIntLimits(1, MAX_STUDY_LENGTH);

		Input_EvaluateOnBarCloseOnly.Name = "Evaluate on Bar Close Only";
		Input_EvaluateOnBarCloseOnly.SetYesNo(false);

		Input_SendTradeOrdersToTradeService.Name = "Send Trade Orders to Trade Service";
		Input_SendTradeOrdersToTradeService.SetYesNo(false);

		Input_MaximumPositionAllowed.Name = "Maximum Position Allowed";
		Input_MaximumPositionAllowed.SetInt(27);

		Input_MaximumLongTradesPerDay.Name = "Maximum Long Trades Per Day";
		Input_MaximumLongTradesPerDay.SetInt(200);

		Input_MaximumShortTradesPerDay.Name = "Maximum Short Trades Per Day";
		Input_MaximumShortTradesPerDay.SetInt(200);

		Input_CancelAllWorkingOrdersOnExit.Name = "Cancel All Working Orders On Exit";
		Input_CancelAllWorkingOrdersOnExit.SetYesNo(false);

		Input_EnableDebuggingOutput.Name = "Enable Debugging Output";
		Input_EnableDebuggingOutput.SetYesNo(true);

		Input_AllowTradingOnlyDuringTimeRange.Name = "Allow Trading Only During Time Range";
		Input_AllowTradingOnlyDuringTimeRange.SetYesNo(true);

		Input_StartTimeForAllowedTimeRange.Name = "Start Time For Allowed Time Range";
		Input_StartTimeForAllowedTimeRange.SetTime(HMS_TIME(9, 30, 0));

		Input_EndTimeForAllowedTimeRange.Name = "End Time For Allowed Time Range";
		Input_EndTimeForAllowedTimeRange.SetTime(HMS_TIME(15, 59, 59));

		Input_AllowOnlyOneTradePerBar.Name = "Allow Only One Trade per Bar";
		Input_AllowOnlyOneTradePerBar.SetYesNo(true);

		Input_SupportReversals.Name = "Support Reversals";
		Input_SupportReversals.SetYesNo(false);

		Input_AllowMultipleEntriesInSameDirection.Name = "Allow Multiple Entries In Same Direction"; 
		Input_AllowMultipleEntriesInSameDirection.SetYesNo(false);

		Input_AllowEntryWithWorkingOrders.Name = "Allow Entry With Working Orders";
		Input_AllowEntryWithWorkingOrders.SetYesNo(false);

		Input_ControlBarButtonNumberForEnableDisable.Name = "ACS Control Bar Button # for Enable/Disable";
		Input_ControlBarButtonNumberForEnableDisable.SetInt(12);
		Input_ControlBarButtonNumberForEnableDisable.SetIntLimits(1, MAX_ACS_CONTROL_BAR_BUTTONS);

		Input_CancelAllOrdersOnEntries.Name = "Cancel All Orders on Entries";
		Input_CancelAllOrdersOnEntries.SetYesNo(false);

		Input_CancelAllOrdersOnReversals.Name = "Cancel All Orders on Reversals";
		Input_CancelAllOrdersOnReversals.SetYesNo(false);

		Input_OrderTextTag.Name = "Order Text Tag";
		Input_OrderTextTag.SetString("delta imbalance");

		Input_Version.SetInt(2);

        Input_DeltaStudyRef.Name = "Delta Study";
        Input_DeltaStudyRef.SetStudySubgraphValues(1, 0); // set default to a likely correct ID

        Input_DeltaThreshold.Name = "Delta Threshold";
        Input_DeltaThreshold.SetInt(100);

        Subgraph_BuyEntry.Name = "Buy";
		Subgraph_BuyEntry.DrawStyle = DRAWSTYLE_POINT_ON_HIGH;
		Subgraph_BuyEntry.LineWidth = 5;

		Subgraph_SellEntry.Name = "Sell";
		Subgraph_SellEntry.DrawStyle = DRAWSTYLE_POINT_ON_LOW;
		Subgraph_SellEntry.LineWidth = 5;

		sc.AllowMultipleEntriesInSameDirection = false;
		sc.SupportReversals = false;
		sc.AllowOppositeEntryWithOpposingPositionOrOrders = false;

		sc.SupportAttachedOrdersForTrading = false;
		sc.UseGUIAttachedOrderSetting = true;
		sc.CancelAllOrdersOnEntriesAndReversals = false;
		sc.AllowEntryWithWorkingOrders = false;

		// Only 1 trade for each Order Action type is allowed per bar.
		sc.AllowOnlyOneTradePerBar = true;

		//This should be set to true when a trading study uses trading functions.
		sc.MaintainTradeStatisticsAndTradesData = true;

		return;
	}

    CrossedLevelStore* store = (CrossedLevelStore*)sc.GetPersistentPointer(1);
    if (store == nullptr)
    {
        store = new CrossedLevelStore;
        sc.SetPersistentPointer(1, store);
    }

    auto ClearLevels = [&]() {
        for (int i = 0; i < store->Count; ++i)
        {
            store->Levels[i] = 0.0;
        }
        store->Count = 0;
    };

    auto IsLevelAlreadyCrossed = [&](double level) -> bool {
        for (int i = 0; i < store->Count; ++i)
        {
            if (fabs(store->Levels[i] - level) < 0.01) // fuzz tolerance
                return true;
        }
        return false;
    };

    auto MarkLevelAsCrossed = [&](double level) {
        if (store->Count < MAX_CROSSED_LEVELS)
            store->Levels[store->Count++] = level;
    };

    if (sc.UpdateStartIndex == 0)
    {
        ClearLevels();
    }

    if (Input_Version.GetInt() < 2)
	{
		Input_AllowOnlyOneTradePerBar.SetYesNo(true);
		Input_Version.SetInt(2);
	}

	sc.AllowOnlyOneTradePerBar = Input_AllowOnlyOneTradePerBar.GetYesNo();
	sc.SupportReversals = false;
	sc.MaximumPositionAllowed = Input_MaximumPositionAllowed.GetInt();
	sc.SendOrdersToTradeService = Input_SendTradeOrdersToTradeService.GetYesNo();
	sc.CancelAllWorkingOrdersOnExit = Input_CancelAllWorkingOrdersOnExit.GetYesNo();

	sc.CancelAllOrdersOnEntries = Input_CancelAllOrdersOnEntries.GetYesNo();
	sc.CancelAllOrdersOnReversals = Input_CancelAllOrdersOnReversals.GetYesNo();

	sc.SupportReversals = Input_SupportReversals.GetYesNo();
	sc.AllowMultipleEntriesInSameDirection = Input_AllowMultipleEntriesInSameDirection.GetYesNo();
	sc.AllowEntryWithWorkingOrders = Input_AllowEntryWithWorkingOrders.GetYesNo();

	if (sc.MenuEventID == Input_ControlBarButtonNumberForEnableDisable.GetInt())
	{
		const int ButtonState = (sc.PointerEventType == SC_ACS_BUTTON_ON) ? 1 : 0;
		Input_Enabled.SetYesNo(ButtonState);
	}


	if (!Input_Enabled.GetYesNo() || sc.LastCallToFunction)
		return;

	int& r_LastDebugMessageType = sc.GetPersistentInt(1);
	int& r_PriorFormulaState = sc.GetPersistentInt(2);

	if (sc.IsFullRecalculation)
	{
		r_LastDebugMessageType = 0;
		r_PriorFormulaState = 0;

		// set ACS Tool Control Bar tool tip
		sc.SetCustomStudyControlBarButtonHoverText(Input_ControlBarButtonNumberForEnableDisable.GetInt(), "Enable/Disable Trade System Based on Alert Condition");
		
		sc.SetCustomStudyControlBarButtonEnable(Input_ControlBarButtonNumberForEnableDisable.GetInt(), Input_Enabled.GetBoolean());
	}


	// Figure out the last bar to evaluate
	int LastIndex = sc.ArraySize - 1;
	if (Input_EvaluateOnBarCloseOnly.GetYesNo())
		--LastIndex;

	const int EarliestIndexToCalculate = LastIndex - Input_NumBarsToCalculate.GetInt() + 1;

	int CalculationStartIndex = sc.UpdateStartIndex;// sc.GetCalculationStartIndexForStudy();

	if (CalculationStartIndex < EarliestIndexToCalculate)
		CalculationStartIndex = EarliestIndexToCalculate;

	sc.EarliestUpdateSubgraphDataArrayIndex = CalculationStartIndex;

	s_SCPositionData PositionData;

	n_ACSIL::s_TradeStatistics TradeStatistics;

	SCString TradeNotAllowedReason;

	const bool IsDownloadingHistoricalData = sc.ChartIsDownloadingHistoricalData(sc.ChartNumber) != 0;

	bool IsTradeAllowed = !sc.IsFullRecalculation && !IsDownloadingHistoricalData;

	if (Input_EnableDebuggingOutput.GetYesNo())
	{
		if (sc.IsFullRecalculation)
			TradeNotAllowedReason = "Is full recalculation";
		else if (IsDownloadingHistoricalData)
			TradeNotAllowedReason = "Downloading historical data";
	}

	if (Input_AllowTradingOnlyDuringTimeRange.GetYesNo())
	{
		const SCDateTime  LastBarDateTime = sc.LatestDateTimeForLastBar;

		const int LastBarTimeInSeconds = LastBarDateTime.GetTimeInSeconds();

		bool LastIndexIsInAllowedTimeRange = false;

		if (Input_StartTimeForAllowedTimeRange.GetTime() <= Input_EndTimeForAllowedTimeRange.GetTime())
		{
			LastIndexIsInAllowedTimeRange = (LastBarTimeInSeconds >= Input_StartTimeForAllowedTimeRange.GetTime()
				&& LastBarTimeInSeconds <= Input_EndTimeForAllowedTimeRange.GetTime()
				);
		}
		else //Reversed times.
		{
			LastIndexIsInAllowedTimeRange = (LastBarTimeInSeconds >= Input_StartTimeForAllowedTimeRange.GetTime()
				|| LastBarTimeInSeconds <= Input_EndTimeForAllowedTimeRange.GetTime()
				);
		}

		if (!LastIndexIsInAllowedTimeRange)
		{
			IsTradeAllowed = false;
			TradeNotAllowedReason = "Not within allowed time range";
		}
	}

	bool ParseAndSetFormula = sc.IsFullRecalculation;
	SCString DateTimeString;

	// Evaluate each of the bars
	for (int BarIndex = CalculationStartIndex; BarIndex <= LastIndex; ++BarIndex)
	{
        DateTimeString = sc.DateTimeToString(sc.BaseDateTimeIn[BarIndex], FLAG_DT_COMPLETE_DATETIME_US);

        SCFloatArray DeltaArray;
        int success = sc.GetStudyArrayUsingID(Input_DeltaStudyRef.GetStudyID(), Input_DeltaStudyRef.GetSubgraphIndex(), DeltaArray);
        if (success == 0 || BarIndex >= DeltaArray.GetArraySize())
            continue; // the array is invalid

        float DeltaValue = DeltaArray[BarIndex];

        const int SourceChart = 9;
        int DrawingIndex = 0;
        s_UseTool Tool;
        bool CrossedAnyLevel = false;
        double Level = 0.0;

        // If we are not evaluating on bar close, skip this bar if it is not the last one
        if (!Input_EvaluateOnBarCloseOnly.GetYesNo() && BarIndex < LastIndex)
            continue;

        // check if we've crossed any of our levels
        // draw levels on SourceChart to monitor them
        // remove them to stop monitoring
        // any monitored level could turn into a trade
        // marking a level on SourceChart means "I want this trade if it sets up"
        while (sc.GetUserDrawnChartDrawing(SourceChart, DRAWING_HORIZONTAL_RAY, Tool, DrawingIndex) != 0)
        {
            Level = Tool.BeginValue;

            // levels should be manually removed ASAP
            // but this will backstop it if we're too slow
            if (IsLevelAlreadyCrossed(Level))
            {
                ++DrawingIndex;
                continue;
            }

            if (sc.High[BarIndex] > Level && sc.Low[BarIndex] < Level)
            {
                MarkLevelAsCrossed(Level);
                CrossedAnyLevel = true;
                break;
            }

            ++DrawingIndex;
        }

        // Skip this candle if no level was crossed
        if (!CrossedAnyLevel)
            continue;

        sc.GetTradePosition(PositionData);
        sc.GetTradeStatisticsForSymbolV2(n_ACSIL::STATS_TYPE_DAILY_ALL_TRADES, TradeStatistics);

        bool IsLongEntry = CrossedAnyLevel && DeltaValue < Input_DeltaThreshold.GetInt() * -1;
        bool IsShortEntry = CrossedAnyLevel && DeltaValue > Input_DeltaThreshold.GetInt();

        bool CanTrade = PositionData.PositionQuantityWithAllWorkingOrders == 0
            || Input_AllowMultipleEntriesInSameDirection.GetYesNo()
            || Input_SupportReversals.GetYesNo();

        SCString DebugMessage;
        if (Input_EnableDebuggingOutput.GetYesNo())
        {
            DebugMessage.Format("BarIndex: %d, Level: %.2f, DeltaValue: %.2f, BarDateTime: %s, IsLongEntry: %s, IsShortEntry: %s, CanTrade: %s", BarIndex, Level, DeltaValue, DateTimeString.GetChars(), IsLongEntry ? "true" : "false", IsShortEntry ? "true" : "false", CanTrade ? "true" : "false");
            sc.AddMessageToTradeServiceLog(DebugMessage, false, true);
        }
        
        if (CanTrade && (IsLongEntry || IsShortEntry))
        {
            s_SCNewOrder NewOrder;
            NewOrder.OrderQuantity = sc.TradeWindowOrderQuantity;
            NewOrder.OrderType = SCT_ORDERTYPE_MARKET;
            NewOrder.TextTag = Input_OrderTextTag.GetString();

            int Result = 0;

            if (IsLongEntry)
            {
                if (TradeStatistics.LongTrades < Input_MaximumLongTradesPerDay.GetInt())
                {
                    Result = static_cast<int>(sc.BuyEntry(NewOrder, BarIndex));
                    Subgraph_BuyEntry[BarIndex] = 1;
                    Subgraph_SellEntry[BarIndex] = 0;
                }
                else
                {
                    if (Input_EnableDebuggingOutput.GetYesNo())
                        sc.AddMessageToTradeServiceLog("Max Long Trades reached", false, true);
                }
            }
            else if (IsShortEntry)
            {
                if (TradeStatistics.ShortTrades < Input_MaximumShortTradesPerDay.GetInt())
                {
                    Result = static_cast<int>(sc.SellEntry(NewOrder, BarIndex));
                    Subgraph_BuyEntry[BarIndex] = 0;
                    Subgraph_SellEntry[BarIndex] = 1;
                }
                else
                {
                    if (Input_EnableDebuggingOutput.GetYesNo())
                        sc.AddMessageToTradeServiceLog("Max Short Trades reached", false, true);
                }
            }

            if (Result < 0)
            {
                sc.AddMessageToTradeServiceLog(sc.GetTradingErrorTextMessage(Result), false, true);
            }
            else
            {
                if (Input_EnableDebuggingOutput.GetYesNo())
                {
                    DebugMessage.Format("Trade executed: %s at %.2f, BarIndex: %d, DateTime: %s",
                        IsLongEntry ? "Buy" : "Sell",
                        sc.Close[BarIndex],
                        DateTimeString.GetChars()
                    );
                    sc.AddMessageToTradeServiceLog(DebugMessage, false, true);
                }
            }
        }
    }
}
