// Reconstructed from the supplied SWF. Descriptive parameter/local names are
// inferred for readability; class/member names and behavior come from bytecode.
package
{
   import Shared.AS3.BSUIComponent;
   import flash.display.MovieClip;

   public class ActiveEffectsWidget extends BSUIComponent
   {
      
      public var IconBackground_mc:MovieClip;
      
      public var Icon_mc:MovieClip;
      
      public var TimerIcon_mc:MovieClip;
      
      public var EffectsBackground_mc:MovieClip;
      
      public var EntryHolder_mc:MovieClip;
      
      private var _EffectType:uint;
      
      private var _EffectsA:Array;
      
      private var _ShowingEffects:Boolean;
      
      private var _ShowingTimer:Boolean;
      
      private var _EntryClipsA:Vector.<ActiveEffects_Entry>;
      
      private var ORIGINAL_HEIGHT:Number;
      
      public function ActiveEffectsWidget()
      {
         super();
         this._EffectType = uint.MAX_VALUE;
         this._ShowingEffects = false;
         this._ShowingTimer = false;
         this._EntryClipsA = new Vector.<ActiveEffects_Entry>();
         this.ORIGINAL_HEIGHT = this.height;
      }
      
      public function set effectsType(effectType:uint) : *
      {
         this._EffectType = effectType;
         this.RefreshEntries();
         SetIsDirty();
      }
      
      public function set effectList(effects:Array) : *
      {
         this._EffectsA = effects;
         this.RefreshEntries();
         SetIsDirty();
      }
      
      public function set showingEffects(showEffects:Boolean) : *
      {
         this._ShowingEffects = showEffects;
         SetIsDirty();
      }
      
      private function RefreshEntries() : *
      {
         var effect:Object = null;
         var entry:ActiveEffects_Entry = null;
         this._EntryClipsA.splice(0,this._EntryClipsA.length);
         this._ShowingTimer = false;
         if(this._EffectsA != null)
         {
            for each(effect in this._EffectsA)
            {
               if(effect.type == this._EffectType)
               {
                  entry = new ActiveEffects_Entry();
                  entry.sourceText = effect.text;
                  entry.effectsList = effect.effects;
                  if(entry.hasDuration)
                  {
                     this._ShowingTimer = true;
                  }
                  this._EntryClipsA.push(entry);
               }
            }
         }
      }
      
      override public function redrawUIComponent() : void
      {
         var nextEntryY:Number;
         var entry:ActiveEffects_Entry = null;
         var backgroundPadding:Number;
         super.redrawUIComponent();
         this.Icon_mc.gotoAndStop(this._EffectType);
         while(this.EntryHolder_mc.numChildren > 0)
         {
            this.EntryHolder_mc.removeChildAt(0);
         }
         if(this._ShowingEffects)
         {
            nextEntryY = 0;
            for each(entry in this._EntryClipsA)
            {
               if(entry.bIsDirty)
               {
                  entry.redrawUIComponent();
               }
               this.EntryHolder_mc.addChild(entry);
               entry.y = nextEntryY;
               nextEntryY += entry.height;
            }
            backgroundPadding = 20;
            this.EffectsBackground_mc.height = this.EntryHolder_mc.height + backgroundPadding;
            this.IconBackground_mc.height = this.EffectsBackground_mc.height;
         }
         else
         {
            this.EffectsBackground_mc.height = this.EntryHolder_mc.height;
            this.IconBackground_mc.height = this.ORIGINAL_HEIGHT;
         }
         this.EffectsBackground_mc.visible = this._ShowingEffects;
         this.EntryHolder_mc.visible = this._ShowingEffects;
         this.TimerIcon_mc.visible = this._ShowingTimer;
      }
   }
}

