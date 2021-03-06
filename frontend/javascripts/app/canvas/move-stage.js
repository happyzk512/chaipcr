/*
 * Chai PCR - Software platform for Open qPCR and Chai's Real-Time PCR instruments.
 * For more information visit http://www.chaibio.com
 *
 * Copyright 2016 Chai Biotechnologies Inc. <info@chaibio.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

angular.module("canvasApp").factory('moveStageRect', [
  'ExperimentLoader',

  function(ExperimentLoader) {

    return {

      getMoveStageRect: function(me) {

        this.currentHit = 0;
        this.currentDrop = null;
        this.startPosition = 0;
        this.endPosition = 0;

        var smallCircle = new fabric.Circle({
          radius: 6, fill: 'white', stroke: "black", strokeWidth: 2, selectable: false,
          left: 69, top: 390, originX: 'center', originY: 'center',
        });

        var smallCircleTop = new fabric.Circle({
          fill: '#FFB300', radius: 6, strokeWidth: 3, selectable: false, stroke: "black",
          left: 69, top: 64, originX: 'center', originY: 'center',
        });

        var stageName = new fabric.Text(
          "STAGE 2", {
            fill: 'black',  fontSize: 12, selectable: false, originX: 'left', originY: 'top',
            top: 15, left: 35, fontFamily: "dinot-bold"
          }
        );

        var stageType = new fabric.Text(
          "HOLDING", {
            fill: 'black',  fontSize: 12, selectable: false, originX: 'left', originY: 'top',
            top: 30, left: 35, fontFamily: "dinot-regular"
          }
        );

        var verticalLine = new fabric.Line([0, 0, 0, 336],{
          left: 68, top: 58, stroke: 'black', strokeWidth: 2, originX: 'left', originY: 'top',
        });

        var rect = new fabric.Rect({
          fill: 'white', width: 135, left: 0, height: 58, selectable: false, name: "step", me: this, rx: 1,
        });

        var coverRect = new fabric.Rect({
          fill: null, width: 135, left: 0, top: 0, height: 372, selectable: false, me: this, rx: 1,
        });

        me.imageobjects["drag-stage-image.png"].originX = "left";
        me.imageobjects["drag-stage-image.png"].originY = "top";
        me.imageobjects["drag-stage-image.png"].top = 15;
        me.imageobjects["drag-stage-image.png"].left = 14;

        indicatorRectangle = new fabric.Group([
          rect, stageName, stageType,
          me.imageobjects["drag-stage-image.png"],
        ],
          {
            originX: "left", originY: "top", left: 0, top: 0, height: 72, selectable: true, lockMovementY: true, hasControls: false,
            visible: true, hasBorders: false, name: "dragStepGroup"
          }
        );

        this.indicator = new fabric.Group([coverRect, indicatorRectangle, verticalLine, smallCircle, smallCircleTop], {
          originX: "left", originY: "top", left: 38, top: 0, height: 372, width: 135, selectable: true,
          lockMovementY: true, hasControls: false, visible: false, hasBorders: false, name: "dragStageGroup"
        });


        this.indicator.init = function(stage) {
          this.draggedStage = stage;
        };

        this.indicator.onTheMoveDragGroup = function(dragging) {
            this.setLeft(dragging.left - 1);
            this.setCoords();
        };

        this.indicator.changePlacing = function(place) {
          this.setVisible(true);
        };

        this.indicator.changeText = function(stage) {

          stageName.setText(stage.stageCaption.text);
          stageType.setText(stage.model.stage_type.toUpperCase());
        };

        this.indicator.onTheMove = function(C) {
          // Here we hit test the movement of the MOVING STAGE
        }

        this.indicator.processMovement = function() {
          // Process movement here
        };

        return this.indicator;
      },

    };
  }
]
);
