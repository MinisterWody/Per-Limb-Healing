// Reconstructed from the supplied SWF. Descriptive parameter/local names are
// inferred for readability; class/member names and behavior come from bytecode.
package
{
   import Shared.AS3.BSButtonHintData;
   import Shared.AS3.BSScrollingList;
   import Shared.AS3.COMPANIONAPP.CompanionAppMode;
   import Shared.AS3.COMPANIONAPP.PipboyLoader;
   import Shared.BGSExternalInterface;
   import Shared.GlobalFunc;
   import flash.display.MovieClip;
   import flash.events.Event;
   import flash.events.IOErrorEvent;
   import flash.net.URLRequest;
   import flash.system.ApplicationDomain;
   import flash.system.LoaderContext;
   import flash.text.TextField;
   import scaleform.gfx.Extensions;
   import scaleform.gfx.TextFieldEx;

   public class PerksTab extends PipboyTab
   {
      
      public var List_mc:BSScrollingList;
      
      public var Description_tf:TextField;
      
      public var StarHolder_mc:MovieClip;
      
      public var VBHolder_mc:MovieClip;
      
      private var _VBLoader:PipboyLoader;
      
      private var _VBClipID:uint;
      
      private var _VBFileName:String;
      
      private var _DescText:String;
      
      private var _CurrSelectedRank:uint;
      
      private var _MaxSelectedRank:uint;
      
      private var ORIG_STAR_X:Number;
      
      private var _ViewingRankOffset:int;
      
      private var _ViewingRanks:Boolean;
      
      private var PrevPerkButton:BSButtonHintData;
      
      private var NextPerkButton:BSButtonHintData;
      
      public function PerksTab()
      {
         this.PrevPerkButton = new BSButtonHintData("$PREV PERK","Ctrl","PSN_L1","Xenon_L1",1,this.onPrevPerk);
         this.NextPerkButton = new BSButtonHintData("$NEXT PERK","Alt","PSN_R1","Xenon_R1",1,this.onNextPerk);
         super();
         this.List_mc.addEventListener(BSScrollingList.SELECTION_CHANGE,this.onListSelectionChange);
         Extensions.enabled = true;
         TextFieldEx.setTextAutoSize(this.Description_tf,TextFieldEx.TEXTAUTOSZ_SHRINK);
         this._VBLoader = new PipboyLoader();
         this._VBClipID = 0;
         this._VBFileName = "";
         this._DescText = "";
         this._CurrSelectedRank = 0;
         this._MaxSelectedRank = 0;
         this._ViewingRankOffset = 0;
         this._ViewingRanks = false;
         this.ORIG_STAR_X = this.StarHolder_mc.x;
         this.__setProp_List_mc_PerksTab_List_0();
      }
      
      override public function PopulateButtonHintData(buttonHints:Vector.<BSButtonHintData>) : *
      {
         buttonHints.push(this.PrevPerkButton);
         buttonHints.push(this.NextPerkButton);
      }
      
      override protected function GetUpdateMask() : PipboyUpdateMask
      {
         return PipboyUpdateMask.Perks;
      }
      
      override protected function onPipboyChangeEvent(event:PipboyChangeEvent) : void
      {
         var wasVisible:Boolean = this.visible == true;
         super.onPipboyChangeEvent(event);
         var isVisible:Boolean = this.visible == true;
         if(CompanionAppMode.isOn)
         {
            if(!wasVisible && isVisible)
            {
               this.List_mc.scrollList.needFullRefresh = true;
            }
         }
         this.List_mc.entryList = event.DataObj.PerksList;
         this.List_mc.entryList.sortOn("text");
         this.List_mc.InvalidateData();
         if(isVisible)
         {
            stage.focus = this.List_mc;
            if(this.List_mc.selectedIndex == -1)
            {
               this.List_mc.selectedClipIndex = 0;
            }
            else
            {
               this.onListSelectionChange();
            }
         }
         else
         {
            this.List_mc.selectedIndex = -1;
         }
         if(isVisible && !wasVisible)
         {
            BGSExternalInterface.call(this.codeObj,"onPerksTabOpen");
         }
         else if(wasVisible && !isVisible)
         {
            BGSExternalInterface.call(this.codeObj,"onPerksTabClose");
         }
         this.PrevPerkButton.ButtonVisible = this.visible;
         this.NextPerkButton.ButtonVisible = this.visible;
         SetIsDirty();
      }
      
      override public function redrawUIComponent() : void
      {
         var request:URLRequest = null;
         var loaderContext:LoaderContext = null;
         var rankIndex:uint = 0;
         var rankStar:MovieClip = null;
         super.redrawUIComponent();
         if(!this._ViewingRanks)
         {
            if(this._VBClipID != 0)
            {
               request = new URLRequest(this._VBFileName);
               loaderContext = new LoaderContext(false,ApplicationDomain.currentDomain);
               this._VBLoader.contentLoaderInfo.addEventListener(Event.COMPLETE,this.onVBLoadComplete);
               this._VBLoader.contentLoaderInfo.addEventListener(IOErrorEvent.IO_ERROR,this.onVBLoadFail);
               this._VBLoader.load(request,loaderContext);
               BGSExternalInterface.call(this.codeObj,"PlayPerkSound",this._VBClipID);
            }
            else
            {
               if(this.VBHolder_mc.numChildren > 0)
               {
                  this.VBHolder_mc.removeChildAt(0);
               }
               BGSExternalInterface.call(this.codeObj,"StopPerkSound");
            }
         }
         GlobalFunc.SetText(this.Description_tf,this._DescText,false);
         while(this.StarHolder_mc.numChildren > 0)
         {
            this.StarHolder_mc.removeChildAt(0);
         }
         if(this._MaxSelectedRank > 1)
         {
            rankIndex = 0;
            while(rankIndex < this._MaxSelectedRank)
            {
               rankStar = new PerkRankStar();
               this.StarHolder_mc.addChild(rankStar);
               rankStar.x = 37.5 * rankIndex;
               if(rankIndex < this._CurrSelectedRank)
               {
                  rankStar.gotoAndStop("Full");
               }
               rankIndex++;
            }
            this.StarHolder_mc.x = this.ORIG_STAR_X - this.StarHolder_mc.width / 2;
         }
         this.PrevPerkButton.ButtonEnabled = this._CurrSelectedRank > 1;
         this.NextPerkButton.ButtonEnabled = this._CurrSelectedRank < this._MaxSelectedRank;
      }
      
      public function onListSelectionChange() : *
      {
         if(this.List_mc.selectedIndex != -1)
         {
            this._DescText = this.List_mc.selectedEntry.descriptions[this.List_mc.selectedEntry.rank - 1];
            this._VBClipID = this.List_mc.selectedEntry.clipName;
            if(this.List_mc.selectedEntry.SWFFile != undefined && this.List_mc.selectedEntry.SWFFile.length > 0)
            {
               this._VBFileName = this.List_mc.selectedEntry.SWFFile;
            }
            else
            {
               this._VBFileName = "Components/VaultBoys/Perks/PerkClip_" + this._VBClipID.toString(16) + ".swf";
            }
            this._CurrSelectedRank = this.List_mc.selectedEntry.rank;
            this._MaxSelectedRank = this.List_mc.selectedEntry.maxRank;
         }
         else
         {
            this._DescText = " ";
            this._VBFileName = "";
            this._VBClipID = 0;
            this._CurrSelectedRank = 0;
            this._MaxSelectedRank = 0;
         }
         this._ViewingRankOffset = 0;
         this._ViewingRanks = false;
         SetIsDirty();
      }
      
      private function onPrevPerk() : *
      {
         --this._ViewingRankOffset;
         this._ViewingRanks = true;
         this.nextPrevPerk_Helper();
         SetIsDirty();
      }
      
      private function onNextPerk() : *
      {
         ++this._ViewingRankOffset;
         this._ViewingRanks = true;
         this.nextPrevPerk_Helper();
         SetIsDirty();
      }
      
      private function nextPrevPerk_Helper() : *
      {
         this._DescText = this.List_mc.selectedEntry.descriptions[this.List_mc.selectedEntry.rank + this._ViewingRankOffset - 1];
         this._CurrSelectedRank = this.List_mc.selectedEntry.rank + this._ViewingRankOffset;
         BGSExternalInterface.call(this.codeObj,"PlaySound","UIMenuPrevNext");
      }
      
      override public function ProcessUserEvent(eventName:String, isPressed:Boolean) : Boolean
      {
         var handled:Boolean = false;
         if(!isPressed)
         {
            if(eventName == "PrevPerk" && this.PrevPerkButton.ButtonVisible && this.PrevPerkButton.ButtonEnabled)
            {
               this.onPrevPerk();
               handled = true;
            }
            else if(eventName == "NextPerk" && this.NextPerkButton.ButtonVisible && this.NextPerkButton.ButtonEnabled)
            {
               this.onNextPerk();
               handled = true;
            }
         }
         return handled;
      }
      
      private function onVBLoadComplete(event:Event) : *
      {
         event.target.removeEventListener(Event.COMPLETE,this.onVBLoadComplete);
         event.target.removeEventListener(IOErrorEvent.IO_ERROR,this.onVBLoadFail);
         if(this.VBHolder_mc.numChildren > 0)
         {
            event.target.content.removeEventListener(Event.ENTER_FRAME,this.onPerkSelectionAnimUpdate);
            this.VBHolder_mc.removeChildAt(0);
         }
         this.VBHolder_mc.addChild(event.target.content);
         event.target.content.addEventListener(Event.ENTER_FRAME,this.onPerkSelectionAnimUpdate);
      }
      
      private function onVBLoadFail(event:Event) : *
      {
         event.target.removeEventListener(Event.COMPLETE,this.onVBLoadComplete);
         event.target.removeEventListener(IOErrorEvent.IO_ERROR,this.onVBLoadFail);
         if(this.VBHolder_mc.numChildren > 0)
         {
            this.VBHolder_mc.removeChildAt(0);
         }
      }
      
      protected function onPerkSelectionAnimUpdate(event:Event) : *
      {
         if(event.target.currentFrame == 1 && this._VBClipID != 0)
         {
            BGSExternalInterface.call(this.codeObj,"PlayPerkSound",this._VBClipID);
         }
      }
      
      internal function __setProp_List_mc_PerksTab_List_0() : *
      {
         try
         {
            this.List_mc["componentInspectorSetting"] = true;
         }
         catch(e:Error)
         {
         }
         this.List_mc.listEntryClass = "PerksListEntry";
         this.List_mc.numListItems = 10;
         this.List_mc.restoreListIndex = false;
         this.List_mc.textOption = "Shrink To Fit";
         this.List_mc.verticalSpacing = 0;
         try
         {
            this.List_mc["componentInspectorSetting"] = false;
         }
         catch(e:Error)
         {
         }
      }
   }
}

