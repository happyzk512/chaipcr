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

window.ChaiBioTech.ngApp.directive('experimentItem', [
  '$state',
  '$stateParams',
  '$rootScope',
  'Status',
  'Experiment',
  function($state, $stateParams, $rootScope, Status, Experiment){
    return {
      restrict: 'EA',
      replace: true,
      scope: {
        state: '@stateVal',
        lidOpen: '=lidOpen',
        maxCycle: '=maxCycle',
        showProp: '=showProp',
        experiment: "=exp"
      },
      templateUrl: "app/views/directives/experiment-item.html",

      link: function(scope, elem) {

        scope.runReady = false;

        scope.expID = $stateParams.id;

        scope.$watch('state', function(val) {
          if(val) {
            if(scope.state === "NOT_STARTED") {
              scope.message = "RUN EXPERIMENT";
            } else if(scope.state === "RUNNING") {
              scope.message = "EXPERIMENT STATUS";
            } else if(scope.state === 'COMPLETED') {
              scope.message = "VIEW RESULT";
            }
          }
        });

        $rootScope.$on('sidemenu:toggle', function() {
          scope.runReady = false;
        });

        scope.manageAction = function() {

          if(scope.state === "NOT_STARTED" && !scope.lidOpen) {
            scope.runReady = !scope.runReady;
          } else if(!scope.lidOpen){
            $state.go('run-experiment', {id: $stateParams.id, chart: 'amplification', max_cycle: scope.maxCycle});
          }
        };

        scope.startExp = function() {
          Experiment.startExperiment($stateParams.id).then(function(data) {
            $rootScope.$broadcast('experiment:started', $stateParams.id);
            if($state.is('edit-protocol')) {
              var max_cycle = Experiment.getMaxExperimentCycle(scope.experiment);
              $state.go('run-experiment', {'id': $stateParams.id, 'chart': 'amplification', 'max_cycle': max_cycle});
            }
          });
        };

      }
    };
  }
]);
