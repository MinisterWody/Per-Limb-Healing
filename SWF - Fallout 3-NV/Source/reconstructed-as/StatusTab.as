// Reconstructed from the supplied SWF. Descriptive parameter/local names are
// inferred for readability; class/member names and behavior come from bytecode.
package
{
   import Shared.AS3.BSButtonHintData;
   import Shared.AS3.BSScrollingList;
   import Shared.AS3.COMPANIONAPP.CompanionAppMode;
   import Shared.AS3.COMPANIONAPP.MobileScrollMovieClip;
   import Shared.AS3.ConditionBoy;
   import Shared.BGSExternalInterface;
   import Shared.GlobalFunc;
   import Shared.PlatformChangeEvent;
   import flash.display.MovieClip;
   import flash.display.Sprite;
   import flash.events.Event;
   import flash.events.KeyboardEvent;
   import flash.events.MouseEvent;
   import flash.geom.Rectangle;
   import flash.text.TextField;
   import flash.ui.Keyboard;
   import scaleform.gfx.Extensions;
   import scaleform.gfx.TextFieldEx;

   public class StatusTab extends PipboyTab
   {
      
      private const LIMB_NONE:int = -1;
      
      private const LIMB_HEAD:int = 0;
      
      private const LIMB_TORSO:int = 1;
      
      private const LIMB_LEFT_ARM:int = 2;
      
      private const LIMB_RIGHT_ARM:int = 3;
      
      private const LIMB_LEFT_LEG:int = 4;
      
      private const LIMB_RIGHT_LEG:int = 5;
      
      private var _HoveredLimb:int = -1;

      private const LIMB_OUTLINE_PADDING:Number = 4;

      private const LIMB_OUTLINE_THICKNESS:Number = 2;

      private var _HeadHit:Sprite;

      private var _TorsoHit:Sprite;

      private var _LArmHit:Sprite;

      private var _RArmHit:Sprite;

      private var _LLegHit:Sprite;

      private var _RLegHit:Sprite;

      private var _PaperHeadHit:Sprite;

      private var _PaperTorsoHit:Sprite;

      private var _PaperLArmHit:Sprite;

      private var _PaperRArmHit:Sprite;

      private var _PaperLLegHit:Sprite;

      private var _PaperRLegHit:Sprite;

      private var _ClassicPaperDoll:ClassicPaperDoll;
      
      public var Name_tf:TextField;
      
      public var Head_Meter:Pipboy_Meter;
      
      public var Torso_Meter:Pipboy_Meter;
      
      public var LArm_Meter:Pipboy_Meter;
      
      public var RArm_Meter:Pipboy_Meter;
      
      public var LLeg_Meter:Pipboy_Meter;
      
      public var RLeg_Meter:Pipboy_Meter;
      
      public var ConditionBoy_mc:ConditionBoy;
      
      public var DMGWidget_mc:Stats_DMGDRWidget;
      
      public var DRWidget_mc:Stats_DMGDRWidget;
      
      public var ActiveEffects_mc:MovieClip;
      
      public var ActiveEffects_HideRect:MovieClip;
      
      public var ActiveEffects_MaskRect:MovieClip;
      
      private var _ActiveEffect_Clips:Vector.<ActiveEffectsWidget>;
      
      private var _ShowingActiveEffects:Boolean;
      
      private var _ActiveEffect_OrigY:Number;
      
      private var _ActiveEffect_ScrollDist:Number;
      
      private var _DataObj:Object;
      
      private var _DamageDirty:Boolean;
      
      private var _EffectsDirty:Boolean;
      
      private var StimpakButton:BSButtonHintData;
      
      private var RadawayButton:BSButtonHintData;
      
      private var ActiveEffectsButton:BSButtonHintData;
      
      private var _isDragging:Boolean = false;
      
      private var _lastMouseY:Number = 1.7976931348623157e+308;
      
      private const COMPANION_ACTIVE_EFFECT_MASK_OFFSET:int = -45;
      
      private var _activeEffectsScroll:MobileScrollMovieClip;
      
      public function StatusTab()
      {
         var scrollBounds:Rectangle = null;
         this.StimpakButton = new BSButtonHintData("$Stimpak","E","PSN_A","Xenon_A",1,this.UseStimpak);
         this.RadawayButton = new BSButtonHintData("$Radaway","R","PSN_X","Xenon_X",1,this.UseRadaway);
         this.ActiveEffectsButton = new BSButtonHintData("$SHOW EFFECTS","Q","PSN_R1","Xenon_R1",1,this.ToggleShowingEffects);
         super();
         Extensions.enabled = true;
         TextFieldEx.setTextAutoSize(this.Name_tf,TextFieldEx.TEXTAUTOSZ_SHRINK);

         // Final in-game classic STATUS meter calibration.  These are offsets
         // from Bethesda's original timeline positions.  Apply them before the
         // PLH interaction overlays are created so each hitbox follows its meter.
         this.Torso_Meter.y -= 165.0;
         this.LArm_Meter.y -= 50.0;
         this.RArm_Meter.x += 6.0;
         this.RArm_Meter.y -= 50.0;
         this.RLeg_Meter.x += 6.0;

         // Classic STATUS variant: keep Bethesda's ConditionBoy instance only as
         // the original placement/tint host.  We deliberately never feed it
         // BodyFlags/HeadFlags, so it never loads a body ConditionClip here.
         // (Its constructor still preloads Condition_Head.swf invisibly.)
         this._ClassicPaperDoll = new ClassicPaperDoll();
         this.ConditionBoy_mc.addChild(this._ClassicPaperDoll);
         this._ActiveEffect_Clips = new Vector.<ActiveEffectsWidget>();
         this._ShowingActiveEffects = false;
         this._ActiveEffect_OrigY = this.ActiveEffects_mc.y;
         this._ActiveEffect_ScrollDist = 0.0;
         this._DamageDirty = false;
         this._EffectsDirty = false;
         addEventListener(MouseEvent.MOUSE_WHEEL,this.onMouseWheel);
         this._HeadHit = this.CreateLimbInteractionOverlay(this.Head_Meter);
         this._TorsoHit = this.CreateLimbInteractionOverlay(this.Torso_Meter);
         this._LArmHit = this.CreateLimbInteractionOverlay(this.LArm_Meter);
         this._RArmHit = this.CreateLimbInteractionOverlay(this.RArm_Meter);
         this._LLegHit = this.CreateLimbInteractionOverlay(this.LLeg_Meter);
         this._RLegHit = this.CreateLimbInteractionOverlay(this.RLeg_Meter);

         this._PaperHeadHit = this._ClassicPaperDoll.GetInteractionHit(this.LIMB_HEAD);
         this._PaperTorsoHit = this._ClassicPaperDoll.GetInteractionHit(this.LIMB_TORSO);
         this._PaperLArmHit = this._ClassicPaperDoll.GetInteractionHit(this.LIMB_LEFT_ARM);
         this._PaperRArmHit = this._ClassicPaperDoll.GetInteractionHit(this.LIMB_RIGHT_ARM);
         this._PaperLLegHit = this._ClassicPaperDoll.GetInteractionHit(this.LIMB_LEFT_LEG);
         this._PaperRLegHit = this._ClassicPaperDoll.GetInteractionHit(this.LIMB_RIGHT_LEG);
         this.RegisterPaperDollHit(this._PaperHeadHit);
         this.RegisterPaperDollHit(this._PaperTorsoHit);
         this.RegisterPaperDollHit(this._PaperLArmHit);
         this.RegisterPaperDollHit(this._PaperRArmHit);
         this.RegisterPaperDollHit(this._PaperLLegHit);
         this.RegisterPaperDollHit(this._PaperRLegHit);
         if(CompanionAppMode.isOn)
         {
            this.ActiveEffects_MaskRect.height += this.COMPANION_ACTIVE_EFFECT_MASK_OFFSET;
            this.ActiveEffects_HideRect.height += this.COMPANION_ACTIVE_EFFECT_MASK_OFFSET;
            scrollBounds = new Rectangle(this.ActiveEffects_HideRect.x,this.ActiveEffects_HideRect.y,this.ActiveEffects_HideRect.width,this.ActiveEffects_HideRect.height);
            this._activeEffectsScroll = new MobileScrollMovieClip(this.ActiveEffects_mc,scrollBounds);
         }
      }
      
      override public function PopulateButtonHintData(buttonHints:Vector.<BSButtonHintData>) : *
      {
         buttonHints.push(this.StimpakButton);
         buttonHints.push(this.RadawayButton);
         buttonHints.push(this.ActiveEffectsButton);
      }
      
      override public function onAddedToStage() : void
      {
         super.onAddedToStage();
         stage.addEventListener(KeyboardEvent.KEY_UP,this.onStatusKeyUp);
      }
      
      override protected function GetUpdateMask() : PipboyUpdateMask
      {
         return PipboyUpdateMask.Stats;
      }
      
      private function ToggleShowingEffects() : *
      {
         var effectWidget:ActiveEffectsWidget = null;
         this._ShowingActiveEffects = !this._ShowingActiveEffects;
         for each(effectWidget in this._ActiveEffect_Clips)
         {
            effectWidget.showingEffects = this._ShowingActiveEffects;
         }
         if(this._ShowingActiveEffects)
         {
            stage.addEventListener(KeyboardEvent.KEY_DOWN,this.onStatusTabKeyDown);
         }
         else
         {
            stage.removeEventListener(KeyboardEvent.KEY_DOWN,this.onStatusTabKeyDown);
         }
         this.ActiveEffectsButton.ButtonText = this._ShowingActiveEffects ? "$HIDE EFFECTS" : "$SHOW EFFECTS";
         this._EffectsDirty = true;
         SetIsDirty();
         BGSExternalInterface.call(this.codeObj,"PlaySmallTransition");
      }
      
      override protected function onPipboyChangeEvent(event:PipboyChangeEvent) : void
      {
         var effect:Object = null;
         var effectWidget:*;
         super.onPipboyChangeEvent(event);
         if(event.DataObj.CurrentTab == this.TabIndex && stage.focus != null)
         {
            stage.focus = null;
         }
         this._DataObj = event.DataObj;
         this._ActiveEffect_Clips.splice(0,this._ActiveEffect_Clips.length);
         event.DataObj.ActiveEffects.sortOn("type");
         var previousEffectType:uint = uint.MAX_VALUE;
         for each(effect in event.DataObj.ActiveEffects)
         {
            if(effect.type != previousEffectType)
            {
               effectWidget = new ActiveEffectsWidget();
               effectWidget.effectsType = effect.type;
               effectWidget.effectList = event.DataObj.ActiveEffects;
               effectWidget.showingEffects = this._ShowingActiveEffects;
               this._ActiveEffect_Clips.push(effectWidget);
               previousEffectType = effect.type;
            }
         }
         this.Head_Meter.SetMeter(event.DataObj.HeadCondition,0,100);
         this.LArm_Meter.SetMeter(event.DataObj.LArmCondition,0,100);
         this.RArm_Meter.SetMeter(event.DataObj.RArmCondition,0,100);
         this.LLeg_Meter.SetMeter(event.DataObj.LLegCondition,0,100);
         this.RLeg_Meter.SetMeter(event.DataObj.RLegCondition,0,100);
         this.Torso_Meter.SetMeter(event.DataObj.TorsoCondition,0,100);
         if(event.DataObj.CurrentTab != this.TabIndex)
         {
            this._HoveredLimb = this.LIMB_NONE;
         }
         this.RefreshLimbSelection();
         this._ClassicPaperDoll.SetState(event.DataObj.HeadCondition,event.DataObj.TorsoCondition,event.DataObj.LArmCondition,event.DataObj.RArmCondition,event.DataObj.LLegCondition,event.DataObj.RLegCondition,event.DataObj.HeadFlags,event.DataObj.CurrHP,event.DataObj.MaxHP);
         this._DamageDirty = event.DataObj.CurrentTab == this.TabIndex;
         this._EffectsDirty = event.DataObj.CurrentTab == this.TabIndex;
         this.StimpakButton.ButtonVisible = this.visible;
         this.RadawayButton.ButtonVisible = this.visible;
         this.UpdateStimpakRadawayEnabled();
         this.ActiveEffectsButton.ButtonVisible = this.visible && event.DataObj.ActiveEffects.length > 0;
         if(this._ShowingActiveEffects && event.DataObj.ActiveEffects.length == 0)
         {
            this.ToggleShowingEffects();
         }
         SetIsDirty();
      }
      
      private function CreateLimbInteractionOverlay(meter:Pipboy_Meter) : Sprite
      {
         var bounds:Rectangle = meter.getBounds(this);
         bounds.inflate(this.LIMB_OUTLINE_PADDING,this.LIMB_OUTLINE_PADDING);

         // Leave the original timeline meter completely untouched.  The overlay
         // is a StatusTab child so changing hit testing cannot disturb the meter
         // timeline (or ConditionBoy / Vault Boy artwork).
         meter.mouseEnabled = false;
         meter.mouseChildren = false;

         var hit:Sprite = new Sprite();
         hit.x = bounds.x;
         hit.y = bounds.y;
         hit.mouseChildren = false;
         hit.mouseEnabled = false;
         hit.buttonMode = false;
         hit.useHandCursor = false;
         hit.graphics.beginFill(16777215,0);
         hit.graphics.drawRect(0,0,bounds.width,bounds.height);
         hit.graphics.endFill();

         var outline:Sprite = new Sprite();
         outline.name = "PLH_Outline";
         outline.mouseEnabled = false;
         outline.mouseChildren = false;
         // Draw in white and let the normal Pip-Boy color transform tint it,
         // just like the original UI artwork.
         outline.graphics.lineStyle(this.LIMB_OUTLINE_THICKNESS,16777215,1);
         outline.graphics.drawRect(0,0,bounds.width,bounds.height);
         outline.visible = false;
         hit.addChild(outline);

         hit.addEventListener(MouseEvent.CLICK,this.onLimbHitClick);
         hit.addEventListener(MouseEvent.ROLL_OVER,this.onLimbHitOver);
         hit.addEventListener(MouseEvent.ROLL_OUT,this.onLimbHitOut);
         this.addChild(hit);
         return hit;
      }

      private function RegisterPaperDollHit(hit:Sprite) : void
      {
         if(hit == null)
         {
            return;
         }
         hit.addEventListener(MouseEvent.CLICK,this.onLimbHitClick);
         hit.addEventListener(MouseEvent.ROLL_OVER,this.onLimbHitOver);
         hit.addEventListener(MouseEvent.ROLL_OUT,this.onLimbHitOut);
      }

      private function onLimbHitClick(event:MouseEvent) : void
      {
         var limb:int = this.GetLimbForHit(event.currentTarget as Sprite);
         if(limb != this.LIMB_NONE && this.IsLimbDamaged(limb) && this.StimpakButton.ButtonEnabled && this.AcknowledgeLimbSelection(limb))
         {
            // Consume a genuine Bethesda Stimpak. The native plugin has already armed this
            // exact limb; DrinkPotion + the newly-created StimpakEffect provide the identity
            // and finalized perk-adjusted treatment amount.
            this.UseStimpak();
            this.FinishTargetedStimpakRequest(limb);
         }
         event.stopPropagation();
      }

      private function onLimbHitOver(event:MouseEvent) : void
      {
         var limb:int = this.GetLimbForHit(event.currentTarget as Sprite);
         if(limb != this.LIMB_NONE && this.IsLimbDamaged(limb) && this._HoveredLimb != limb)
         {
            this._HoveredLimb = limb;
            this.RefreshLimbSelection();
            // Match the normal Pip-Boy focus feedback exactly once when the pointer
            // transitions onto a damaged limb.  Clicking remains visually/audio silent
            // apart from Bethesda's native Stimpak use feedback.
            dispatchEvent(new Event(BSScrollingList.PLAY_FOCUS_SOUND,true,true));
         }
      }

      private function onLimbHitOut(event:MouseEvent) : void
      {
         var limb:int = this.GetLimbForHit(event.currentTarget as Sprite);
         if(this._HoveredLimb == limb)
         {
            this._HoveredLimb = this.LIMB_NONE;
            this.RefreshLimbSelection();
         }
      }

      private function GetLimbForHit(hit:Sprite) : int
      {
         if(hit == this._HeadHit || hit == this._PaperHeadHit)
         {
            return this.LIMB_HEAD;
         }
         if(hit == this._TorsoHit || hit == this._PaperTorsoHit)
         {
            return this.LIMB_TORSO;
         }
         if(hit == this._LArmHit || hit == this._PaperLArmHit)
         {
            return this.LIMB_LEFT_ARM;
         }
         if(hit == this._RArmHit || hit == this._PaperRArmHit)
         {
            return this.LIMB_RIGHT_ARM;
         }
         if(hit == this._LLegHit || hit == this._PaperLLegHit)
         {
            return this.LIMB_LEFT_LEG;
         }
         if(hit == this._RLegHit || hit == this._PaperRLegHit)
         {
            return this.LIMB_RIGHT_LEG;
         }
         return this.LIMB_NONE;
      }

      private function GetPerLimbHealingPlugin() : Object
      {
         var f4seObj:Object = null;
         var page:Pipboy_StatsPage = parent as Pipboy_StatsPage;
         if(page !== null)
         {
            f4seObj = page.GetF4SEObject();
         }
         if(f4seObj === null)
         {
            var current:Object = this;
            while(current !== null)
            {
               if("f4se" in current && current.f4se !== null)
               {
                  f4seObj = current.f4se;
                  break;
               }
               current = current.parent;
            }
         }
         if(f4seObj === null || !("plugins" in f4seObj) || f4seObj.plugins === null || !("PerLimbHealing" in f4seObj.plugins) || f4seObj.plugins.PerLimbHealing === null)
         {
            return null;
         }
         return f4seObj.plugins.PerLimbHealing;
      }
      
      private function AcknowledgeLimbSelection(limb:int) : Boolean
      {
         var plugin:Object = this.GetPerLimbHealingPlugin();
         if(plugin === null || !("AcknowledgeLimb" in plugin) || !(plugin.AcknowledgeLimb is Function))
         {
            return false;
         }
         var acknowledgement:* = plugin.AcknowledgeLimb(limb);
         return int(acknowledgement) == limb;
      }
      
      private function FinishTargetedStimpakRequest(limb:int) : void
      {
         var plugin:Object = this.GetPerLimbHealingPlugin();
         if(plugin !== null && "FinishTargetedStimpak" in plugin && plugin.FinishTargetedStimpak is Function)
         {
            plugin.FinishTargetedStimpak(limb);
         }
      }
      
      private function IsLimbDamaged(limb:int) : Boolean
      {
         if(this._DataObj === null)
         {
            return false;
         }
         switch(limb)
         {
            case this.LIMB_HEAD:
               return this._DataObj.HeadCondition < 100;
            case this.LIMB_TORSO:
               return this._DataObj.TorsoCondition < 100;
            case this.LIMB_LEFT_ARM:
               return this._DataObj.LArmCondition < 100;
            case this.LIMB_RIGHT_ARM:
               return this._DataObj.RArmCondition < 100;
            case this.LIMB_LEFT_LEG:
               return this._DataObj.LLegCondition < 100;
            case this.LIMB_RIGHT_LEG:
               return this._DataObj.RLegCondition < 100;
         }
         return false;
      }
      
      private function UpdateLimbHitSelection(hit:Sprite,limb:int) : void
      {
         if(hit == null)
         {
            return;
         }
         var damaged:Boolean = this.IsLimbDamaged(limb);
         hit.mouseEnabled = damaged;
         hit.buttonMode = damaged;
         hit.useHandCursor = damaged;
         var outline:Sprite = hit.getChildByName("PLH_Outline") as Sprite;
         if(outline != null)
         {
            outline.visible = damaged && this._HoveredLimb == limb;
         }
      }

      private function RefreshLimbSelection() : void
      {
         if(this._HoveredLimb != this.LIMB_NONE && !this.IsLimbDamaged(this._HoveredLimb))
         {
            this._HoveredLimb = this.LIMB_NONE;
         }
         this.UpdateLimbHitSelection(this._HeadHit,this.LIMB_HEAD);
         this.UpdateLimbHitSelection(this._TorsoHit,this.LIMB_TORSO);
         this.UpdateLimbHitSelection(this._LArmHit,this.LIMB_LEFT_ARM);
         this.UpdateLimbHitSelection(this._RArmHit,this.LIMB_RIGHT_ARM);
         this.UpdateLimbHitSelection(this._LLegHit,this.LIMB_LEFT_LEG);
         this.UpdateLimbHitSelection(this._RLegHit,this.LIMB_RIGHT_LEG);
         this.UpdateLimbHitSelection(this._PaperHeadHit,this.LIMB_HEAD);
         this.UpdateLimbHitSelection(this._PaperTorsoHit,this.LIMB_TORSO);
         this.UpdateLimbHitSelection(this._PaperLArmHit,this.LIMB_LEFT_ARM);
         this.UpdateLimbHitSelection(this._PaperRArmHit,this.LIMB_RIGHT_ARM);
         this.UpdateLimbHitSelection(this._PaperLLegHit,this.LIMB_LEFT_LEG);
         this.UpdateLimbHitSelection(this._PaperRLegHit,this.LIMB_RIGHT_LEG);
         this._ClassicPaperDoll.SetHoveredLimb(this._HoveredLimb);
      }
      
      public function UpdateStimpakRadawayEnabled() : *
      {
         var savedName:String = null;
         if(this._DataObj !== null)
         {
            this.StimpakButton.ButtonEnabled = this._DataObj.StimpakCount > 0 && !this._DataObj.ReadOnlyMode;
            this.RadawayButton.ButtonEnabled = this._DataObj.RadawayCount > 0 && !this._DataObj.ReadOnlyMode;
            if(CompanionAppMode.isOn)
            {
               savedName = this.Name_tf.text;
               this.Name_tf.text = "$Stimpak";
               this.StimpakButton.ButtonText = this.Name_tf.text + " (" + this._DataObj.StimpakCount.toString() + ")";
               this.Name_tf.text = "$Radaway";
               this.RadawayButton.ButtonText = this.Name_tf.text + " (" + this._DataObj.RadawayCount.toString() + ")";
               this.Name_tf.text = savedName;
            }
            else
            {
               this.StimpakButton.ButtonText = "$$Stimpak (" + this._DataObj.StimpakCount.toString() + ")";
               this.RadawayButton.ButtonText = "$$Radaway (" + this._DataObj.RadawayCount.toString() + ")";
            }
         }
      }
      
      override protected function onReadOnlyChanged(readOnly:Boolean) : void
      {
         super.onReadOnlyChanged(readOnly);
         this.UpdateStimpakRadawayEnabled();
      }
      
      override public function redrawUIComponent() : void
      {
         var widgetSpacing:Number;
         var pageWidth:Number;
         var damageWidgetX:Number;
         var nextEffectY:Number;
         var effectSpacing:Number;
         var effectWidget:ActiveEffectsWidget = null;
         super.redrawUIComponent();
         GlobalFunc.SetText(this.Name_tf,this._DataObj.PlayerName,false);
         if(this._DamageDirty)
         {
            this.DMGWidget_mc.redraw(true,this._DataObj.TotalDamages);
            this.DRWidget_mc.redraw(false,this._DataObj.TotalResists);
            widgetSpacing = 20;
            pageWidth = 876;
            damageWidgetX = (pageWidth - (this.DMGWidget_mc.width + this.DRWidget_mc.width + widgetSpacing)) / 2;
            this.DMGWidget_mc.x = damageWidgetX;
            this.DRWidget_mc.x = this.DMGWidget_mc.x + this.DMGWidget_mc.width + widgetSpacing;
            this._DamageDirty = false;
         }
         if(this._EffectsDirty)
         {
            while(this.ActiveEffects_mc.numChildren > 0)
            {
               this.ActiveEffects_mc.removeChildAt(0);
            }
            nextEffectY = 0;
            effectSpacing = 10;
            for each(effectWidget in this._ActiveEffect_Clips)
            {
               if(effectWidget.bIsDirty)
               {
                  effectWidget.redrawUIComponent();
               }
               this.ActiveEffects_mc.addChild(effectWidget);
               effectWidget.y = nextEffectY;
               nextEffectY += effectWidget.height + effectSpacing;
            }
            if(CompanionAppMode.isOn)
            {
               if(this.ActiveEffects_mc.height > this.ActiveEffects_HideRect.height)
               {
                  this._activeEffectsScroll.activate();
               }
               else
               {
                  this._activeEffectsScroll.deactivate();
                  this.ActiveEffects_mc.y = this._ActiveEffect_OrigY;
               }
            }
            else
            {
               this._ActiveEffect_ScrollDist = Math.max(0.0,this.ActiveEffects_mc.y + nextEffectY - this.ActiveEffects_HideRect.y - this.ActiveEffects_HideRect.height);
            }
            this.ActiveEffects_HideRect.visible = this._ShowingActiveEffects;
            this._EffectsDirty = false;
         }
      }
      
      override public function ProcessUserEvent(eventName:String, isPressed:Boolean) : Boolean
      {
         var handled:Boolean = false;
         if(!isPressed)
         {
            if(eventName == "Accept" && this.iPlatform != PlatformChangeEvent.PLATFORM_PC_KB_MOUSE && this.StimpakButton.ButtonVisible && this.StimpakButton.ButtonEnabled)
            {
               this.UseStimpak();
               handled = true;
            }
            else if(eventName == "XButton" && this.RadawayButton.ButtonVisible && this.RadawayButton.ButtonEnabled)
            {
               this.UseRadaway();
               handled = true;
            }
            else if(eventName == "RShoulder" && this.ActiveEffectsButton.ButtonVisible && this.ActiveEffectsButton.ButtonEnabled)
            {
               this.ToggleShowingEffects();
               handled = true;
            }
         }
         return handled;
      }
      
      private function onStatusKeyUp(event:KeyboardEvent) : *
      {
         if(event.keyCode == Keyboard.E && this.iPlatform == PlatformChangeEvent.PLATFORM_PC_KB_MOUSE && this.StimpakButton.ButtonVisible && this.StimpakButton.ButtonEnabled)
         {
            this.UseStimpak();
            event.stopPropagation();
         }
      }
      
      private function UseStimpak() : void
      {
         BGSExternalInterface.call(this.codeObj,"UseStimpak");
      }
      
      private function UseRadaway() : void
      {
         BGSExternalInterface.call(this.codeObj,"UseRadaway");
      }
      
      private function onStatusTabKeyDown(event:KeyboardEvent) : *
      {
         var previousY:*;
         if(this._ShowingActiveEffects && this.visible && this._ActiveEffect_ScrollDist != 0.0)
         {
            previousY = this.ActiveEffects_mc.y;
            if(event.keyCode == Keyboard.UP)
            {
               this.ScrollActiveEffects(true);
            }
            else if(event.keyCode == Keyboard.DOWN)
            {
               this.ScrollActiveEffects(false);
            }
            if(previousY != this.ActiveEffects_mc.y)
            {
               dispatchEvent(new Event(BSScrollingList.PLAY_FOCUS_SOUND,true,true));
            }
         }
      }
      
      override public function onRightThumbstickInput(direction:uint) : *
      {
         var previousY:*;
         if(this._ShowingActiveEffects && this.visible && this._ActiveEffect_ScrollDist != 0.0)
         {
            previousY = this.ActiveEffects_mc.y;
            if(direction == 1)
            {
               this.ScrollActiveEffects(true);
            }
            else if(direction == 3)
            {
               this.ScrollActiveEffects(false);
            }
            if(previousY != this.ActiveEffects_mc.y)
            {
               dispatchEvent(new Event(BSScrollingList.PLAY_FOCUS_SOUND,true,true));
            }
         }
      }
      
      private function ScrollActiveEffects(scrollUp:Boolean) : *
      {
         var scrollStep:Number = 75;
         this.ScrollActiveEffectsBy(scrollStep * (scrollUp ? 1 : -1));
      }
      
      private function ScrollActiveEffectsBy(distance:Number) : *
      {
         this.ActiveEffects_mc.y += distance;
         this.ActiveEffects_mc.y = Math.min(this.ActiveEffects_mc.y,this._ActiveEffect_OrigY);
         this.ActiveEffects_mc.y = Math.max(this.ActiveEffects_mc.y,this._ActiveEffect_OrigY - this._ActiveEffect_ScrollDist);
      }
      
      public function onMouseWheel(event:MouseEvent) : *
      {
         var previousY:*;
         if(this._ShowingActiveEffects && this.visible && this._ActiveEffect_ScrollDist != 0.0)
         {
            previousY = this.ActiveEffects_mc.y;
            if(event.delta < 0)
            {
               this.ScrollActiveEffects(false);
            }
            else if(event.delta > 0)
            {
               this.ScrollActiveEffects(true);
            }
            event.stopPropagation();
            if(previousY != this.ActiveEffects_mc.y)
            {
               dispatchEvent(new Event(BSScrollingList.PLAY_FOCUS_SOUND,true,true));
            }
         }
      }
      
      override public function onRemovedFromStage() : void
      {
         if(CompanionAppMode.isOn)
         {
            this._activeEffectsScroll.deactivate();
            this._activeEffectsScroll = null;
         }
         super.onRemovedFromStage();
      }
   }
}

