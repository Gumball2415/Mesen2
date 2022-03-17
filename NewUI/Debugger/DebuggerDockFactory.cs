﻿using Avalonia.Data;
using Dock.Avalonia.Controls;
using Dock.Model;
using Dock.Model.ReactiveUI.Controls;
using Dock.Model.Core;
using Mesen.Debugger.ViewModels;
using Mesen.Debugger.ViewModels.DebuggerDock;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Dock.Model.Controls;
using Dock.Model.ReactiveUI;
using Mesen.Debugger.StatusViews;

namespace Mesen.Debugger
{
	public class DebuggerDockFactory : Factory
	{
		public ToolContainerViewModel<DisassemblyViewModel> DisassemblyTool { get; private set; }
		public ToolContainerViewModel<SourceViewViewModel> SourceViewTool { get; private set; }
		public ToolContainerViewModel<BaseConsoleStatusViewModel> StatusTool { get; private set; }
		public ToolContainerViewModel<BreakpointListViewModel> BreakpointListTool { get; private set; }
		public ToolContainerViewModel<WatchListViewModel> WatchListTool { get; private set; }
		public ToolContainerViewModel<CallStackViewModel> CallStackTool { get; private set; }
		public ToolContainerViewModel<LabelListViewModel> LabelListTool { get; private set; }

		public DebuggerDockFactory()
		{
			DisassemblyTool = new("Disassembly");
			DisassemblyTool.CanClose = false;
			SourceViewTool = new("Source View");
			SourceViewTool.CanClose = false;

			StatusTool = new("Status");
			BreakpointListTool = new("Breakpoints");
			WatchListTool = new("Watch");
			CallStackTool = new("Call Stack");
			LabelListTool = new("Labels");
		}

		public override IRootDock CreateLayout()
		{
			var codeDockables = CreateList<IDockable>(DisassemblyTool);
			codeDockables.Add(SourceViewTool);

			var mainLayout = new ProportionalDock {
				Orientation = Orientation.Vertical,
				VisibleDockables = CreateList<IDockable>(
					new ProportionalDock {
						Proportion = 0.75,
						Orientation = Orientation.Horizontal,
						ActiveDockable = null,
						VisibleDockables = CreateList<IDockable>(
							new ToolDock {
								Proportion = 0.60,
								VisibleDockables = codeDockables
							},
							new ProportionalDockSplitter(),
							new ProportionalDock {
								Proportion = 0.40,
								Orientation = Orientation.Vertical,
								VisibleDockables = CreateList<IDockable>(
									new ToolDock {
										Proportion = 0.5,
										VisibleDockables = CreateList<IDockable>(StatusTool)
									},
									new ProportionalDockSplitter(),
									new ToolDock {
										Proportion = 0.5,
										VisibleDockables = CreateList<IDockable>(LabelListTool)
									}
								)
							}
						)
					},
					new ProportionalDockSplitter(),
					new ProportionalDock {
						Proportion = 0.25,
						Orientation = Orientation.Horizontal,
						VisibleDockables = CreateList<IDockable>(
							new ToolDock {
								Proportion = 0.33,
								VisibleDockables = CreateList<IDockable>(WatchListTool)
							},
							new ProportionalDockSplitter(),
							new ToolDock {
								Proportion = 0.33,
								VisibleDockables = CreateList<IDockable>(BreakpointListTool)
							},
							new ProportionalDockSplitter(),
							new ToolDock {
								Proportion = 0.33,
								VisibleDockables = CreateList<IDockable>(CallStackTool)
							}
						)
					}
				)
			};

			var mainView = new DebuggerDockViewModel {
				Id = "Main",
				Title = "Main",
				ActiveDockable = mainLayout,
				VisibleDockables = CreateList<IDockable>(mainLayout)
			};

			var root = CreateRootDock();

			root.Id = "Root";
			root.Title = "Root";
			root.ActiveDockable = mainView;
			root.DefaultDockable = mainView;
			root.VisibleDockables = CreateList<IDockable>(mainView);

			return root;
		}

		public override void InitLayout(IDockable layout)
		{
			this.ContextLocator = new Dictionary<string, Func<object>> {
			};

			this.HostWindowLocator = new Dictionary<string, Func<IHostWindow>> {
				[nameof(IDockWindow)] = () => new HostWindow()
			};

			this.DockableLocator = new Dictionary<string, Func<IDockable?>> { };

			base.InitLayout(layout);
		}
	}
}
