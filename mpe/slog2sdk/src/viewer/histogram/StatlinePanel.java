/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.histogram;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.border.*;
import javax.swing.event.*;

import base.drawable.TimeBoundingBox;
import base.statistics.BufForTimeAveBoxes;
import logformat.slog2.LineIDMap;
import viewer.common.Const;
import viewer.common.Dialogs;
import viewer.common.Parameters;
import viewer.zoomable.Debug;
import viewer.zoomable.ModelTime;
import viewer.zoomable.YaxisMaps;
import viewer.zoomable.YaxisTree;
import viewer.zoomable.ScrollbarTime;
import viewer.zoomable.ModelTimePanel;
import viewer.zoomable.RulerTime;
import viewer.zoomable.ViewportTime;
import viewer.zoomable.ViewportTimeYaxis;
import viewer.zoomable.ViewportTimePanel;
import viewer.zoomable.RowAdjustments;

public class StatlinePanel extends JPanel
{
    private Window                  root_window;
    private TimeBoundingBox         timebox;
    private BufForTimeAveBoxes      buf4statboxes;
    
    private StatlineToolBar         toolbar;
    private StatBoxStatusPanel      status_panel;
    private BoundedRangeModel       y_model;
    private YaxisMaps               y_maps;
    private YaxisTree               y_tree;
    private JScrollPane             y_scroller;
    private JScrollBar              y_scrollbar;


    private ModelTime               time_model;
    private ScrollbarTime           time_scrollbar;
    private ModelTimePanel          time_display_panel;

    private RulerTime               time_ruler;
    private ViewportTime            time_ruler_vport;
    private ViewportTimePanel       time_ruler_panel;

    private CanvasStatline          stat_canvas;
    private ViewportTimeYaxis       stat_canvas_vport;
    private ViewportTimePanel       stat_canvas_panel;


    private SummaryStateComboBox    summary_state_combobox;
    private RowAdjustments          row_adjs;
    private String                  err_msg;

    public StatlinePanel( final Window              parent_window,
                          final TimeBoundingBox     times,
                          final LineIDMap           lineIDmap,
                          final BufForTimeAveBoxes  statboxes )
    {
        super();
        root_window   = parent_window;
        timebox       = times;
        buf4statboxes = statboxes;

        /* Initialize the YaxisMaps through the initialization of YaxisTree */
        String[]      y_colnames = lineIDmap.getColumnLabels();
        y_maps      = new YaxisMaps( lineIDmap );
        y_tree      = new YaxisTree( y_maps.getTreeRoot() );
        y_maps.setTreeView( y_tree );
                    /* done YaxisMaps initialization */

        /*
           y_scroller for y_tree needs to be created before stat_canvas, so
           y_model can be extracted to be used for the creation of stat_canvas
        */
        y_scroller  = new JScrollPane( y_tree,
                          ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS,
                          ScrollPaneConstants.HORIZONTAL_SCROLLBAR_ALWAYS );
        y_scrollbar = y_scroller.getVerticalScrollBar();
        y_model     = y_scrollbar.getModel();

        /* Initialize the ModelTime */
        if ( Parameters.HISTOGRAM_ZERO_ORIGIN )
            time_model    = new ModelTime( root_window,
                                           0.0d, timebox.getDuration() );
        else
            time_model    = new ModelTime( root_window,
                                           timebox.getEarliestTime(),
                                           timebox.getLatestTime() );

        this.setLayout( new BorderLayout() );

            /* Setting up the CENTER panel to store various time-related GUIs */
            JPanel center_panel = new JPanel();
            center_panel.setLayout( new BoxLayout( center_panel,
                                                   BoxLayout.Y_AXIS ) );

                /* The Time Ruler */
                time_ruler        = new RulerTime( time_model );
                time_ruler_vport  = new ViewportTime( time_model );
                time_ruler_vport.setView( time_ruler );
                time_ruler_panel  = new ViewportTimePanel( time_ruler_vport );
                if ( Parameters.HISTOGRAM_ZERO_ORIGIN )
                    time_ruler_panel.setBorderTitle( " Duration (seconds) ",
                                                     TitledBorder.RIGHT,
                                                     TitledBorder.BOTTOM,
                                                     Const.FONT, Color.red );
                else
                    time_ruler_panel.setBorderTitle( " Time (seconds) ",
                                                     TitledBorder.RIGHT,
                                                     TitledBorder.BOTTOM,
                                                     Const.FONT, Color.red );
                time_ruler_vport.initLeftMouseToZoom( false );
                /*
                   Propagation of AdjustmentEvent originating from scroller:

                   scroller -----> time_model -----> viewport -----> view
                             adj               time           paint
                   viewport is between time_model and view because
                   viewport is what user sees.
                */
                time_model.addTimeListener( time_ruler_vport );
                /*
                   Since there is NOT a specific ViewportTime/ViewTimePanel
                   for RulerTime, so we need to set PreferredSize of RulerTime
                   here.  Since CanvasTime's has its MaximumSize set to MAX,
                   CanvasTime's ViewportTimePanel will become space hungary.
                   As we want RulerTime to be fixed height during resize
                   of the top level window, So it becomes CRUCIAL to set
                   Preferred Height of RulerTime's ViewportTimePanel equal
                   to its Minimum Height and Maximum Height.
                */
                Insets   ruler_panel_insets = time_ruler_panel.getInsets();
                int      ruler_panel_height = ruler_panel_insets.top
                                            + time_ruler.getJComponentHeight()
                                            + ruler_panel_insets.bottom;
                time_ruler_panel.setPreferredSize(
                     new Dimension( 100, ruler_panel_height ) );

                /* The StatLine Canvas */
                stat_canvas       = new CanvasStatline( time_model,
                                                        buf4statboxes,
                                                        y_model, y_maps,
                                                        y_colnames );
                stat_canvas_vport = new ViewportTimeYaxis( time_model,
                                                           y_model, y_tree );
                stat_canvas_vport.setView( stat_canvas );
                stat_canvas_panel = new ViewportTimePanel( stat_canvas_vport );
                stat_canvas_panel.setBorderTitle( " Statistics ",
                                                  TitledBorder.RIGHT,
                                                  TitledBorder.TOP,
                                                  null, Color.blue );
                stat_canvas_vport.initLeftMouseToZoom( true );
                /* Inform "stat_canvas_vport" time has been changed */
                time_model.addTimeListener( stat_canvas_vport );

                /* The View's Time Display Panel */
                time_display_panel = new ModelTimePanel( time_model );
                time_model.setParamDisplay( time_display_panel );
                time_display_panel.addViewportTime( time_ruler_vport );
                time_display_panel.addViewportTime( stat_canvas_vport );
                JPanel canvas_lmouse;
                canvas_lmouse = stat_canvas_vport.createLeftMouseModePanel(
                                                            BoxLayout.X_AXIS );
                canvas_lmouse.setToolTipText(
                "Operation for left mouse button click on Statistics canvas" );
                time_display_panel.add( canvas_lmouse );

                /* The Horizontal "Time" ScrollBar */
                time_scrollbar = new ScrollbarTime( time_model );
                time_scrollbar.setEnabled( true );
                time_model.setScrollBar( time_scrollbar );

            center_panel.add( time_display_panel );
            center_panel.add( stat_canvas_panel );
            center_panel.add( time_scrollbar );
            center_panel.add( time_ruler_panel );

            /* Setting up the LEFT panel to store various Y-axis related GUIs */
            JPanel left_panel = new JPanel();
            left_panel.setLayout( new BoxLayout( left_panel,
                                                 BoxLayout.Y_AXIS ) );
                // Status of the BufForTimeAveBoxes
                status_panel = new StatBoxStatusPanel( buf4statboxes );
                int panel_height = time_display_panel.getPreferredSize().height;
                status_panel.setMinimumSize(
                  new Dimension( 85, panel_height ) );
                status_panel.setMaximumSize(
                  new Dimension( Short.MAX_VALUE, panel_height ) );
                status_panel.setPreferredSize(
                  new Dimension( 85, panel_height ) );
                status_panel.setAlignmentX( Component.CENTER_ALIGNMENT );

                /* "VIEW" title */
                Insets canvas_panel_insets = stat_canvas_panel.getInsets();
                summary_state_combobox = new SummaryStateComboBox();
                summary_state_combobox.setMinimumSize(
                  new Dimension( 0, canvas_panel_insets.top ) );
                summary_state_combobox.setMaximumSize(
                  new Dimension( Short.MAX_VALUE, canvas_panel_insets.top ) );
                summary_state_combobox.setPreferredSize(
                  new Dimension( 20, canvas_panel_insets.top ) );
                summary_state_combobox.setAlignmentX(
                                       Component.CENTER_ALIGNMENT );

                /* YaxisTree View */
                y_scroller.setAlignmentX( Component.CENTER_ALIGNMENT );
                /* when y_scrollbar is changed, update stat_canvas as well. */
                y_scrollbar.addAdjustmentListener( stat_canvas_vport );

                /* YaxisTree's Column Labels */
                int       left_bottom_height = ruler_panel_height
                                             + canvas_panel_insets.bottom;
                JTextArea y_colarea   = new JTextArea();
                // y_colarea.setFont( Const.FONT );
                StringBuffer text_space  = new StringBuffer( " " );
                for ( int idx = 0; idx < y_colnames.length; idx++ ) {
                    y_colarea.append( text_space.toString() + "@ "
                                    + y_colnames[ idx ] + "\n" );
                    text_space.append( "    " );
                }
                JScrollPane y_colpanel = new JScrollPane( y_colarea,
                          ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS,
                          ScrollPaneConstants.HORIZONTAL_SCROLLBAR_ALWAYS );
                /*
                   Since there is NOT a specific Top Level JPanel for
                   y_colpanel, so we need to set its PreferredSize here.
                   Since y_scroller(i.e. JScrollPane containing YaxisTree)
                   is the space hungary component here.  So it is CRUCIAL
                   to set the height PreferredSize equal to that of MinimumSize
                   and MaximumSize, hence y_colpanel will be fixed in height
                   during resizing of the top level frame.
                */
                y_colpanel.setMinimumSize(
                     new Dimension( 0, left_bottom_height ) );
                y_colpanel.setMaximumSize(
                     new Dimension( Short.MAX_VALUE, left_bottom_height ) );
                y_colpanel.setPreferredSize(
                     new Dimension( 20, left_bottom_height ) );

            left_panel.add( status_panel );
            left_panel.add( summary_state_combobox );
            left_panel.add( y_scroller );
            left_panel.add( y_colpanel );

            /* Setting up the RIGHT panel to store various time-related GUIs */
            JPanel right_panel = new JPanel();
            right_panel.setLayout( new BoxLayout( right_panel,
                                                  BoxLayout.Y_AXIS ) );
                row_adjs = new RowAdjustments( stat_canvas_vport, y_tree );

                JPanel row_resize  = row_adjs.getComboBoxPanel();
                row_resize.setMinimumSize(
                    new Dimension( 0, canvas_panel_insets.top ) );
                row_resize.setMaximumSize(
                    new Dimension( Short.MAX_VALUE, canvas_panel_insets.top ) );
                row_resize.setPreferredSize(
                    new Dimension( 20, canvas_panel_insets.top ) );
                row_resize.setAlignmentX( Component.CENTER_ALIGNMENT );

                JPanel row_txtfld  = row_adjs.getTextFieldPanel();
                row_txtfld.setAlignmentX( Component.CENTER_ALIGNMENT );

                JPanel row_slider  = row_adjs.getSliderPanel();
                JScrollPane slider_scroller = new JScrollPane( row_slider,
                            ScrollPaneConstants.VERTICAL_SCROLLBAR_NEVER,
                            ScrollPaneConstants.HORIZONTAL_SCROLLBAR_ALWAYS );
                slider_scroller.setAlignmentX( Component.CENTER_ALIGNMENT );

                JPanel row_misc  = row_adjs.getMiscPanel();
                row_misc.setAlignmentX( Component.CENTER_ALIGNMENT );
                JPanel ruler_lmouse;
                ruler_lmouse = time_ruler_vport.createLeftMouseModePanel(
                                                      BoxLayout.X_AXIS );
                ruler_lmouse.setToolTipText(
                "Operation for left mouse button click on Time Ruler" );
                ruler_lmouse.setAlignmentX( Component.LEFT_ALIGNMENT );
                row_misc.add( ruler_lmouse );

                JScrollPane row_colpanel = new JScrollPane( row_misc,
                          ScrollPaneConstants.VERTICAL_SCROLLBAR_AS_NEEDED,
                          ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED );
                /*
                   Since there is NOT a specific Top Level JPanel for
                   row_colpanel, so we need to set its PreferredSize here.
                   Since slider_scroller(i.e. JScrollPane containing JSlider)
                   is the space hungary component here.  So it is CRUCIAL
                   to set the height PreferredSize equal to that of MinimumSize
                   and MaximumSize, hence slider_scroller will be fixed in
                   height during resizing of the top level frame.
                */
                row_colpanel.setMinimumSize(
                    new Dimension( 0, left_bottom_height ) );
                row_colpanel.setMaximumSize(
                    new Dimension( Short.MAX_VALUE, left_bottom_height ) );
                row_colpanel.setPreferredSize(
                    new Dimension( 20, left_bottom_height ) );
            right_panel.add( row_resize );
            right_panel.add( row_txtfld );
            right_panel.add( slider_scroller );
            right_panel.add( row_colpanel );


            /* Store the LEFT and CENTER panels in the JSplitPane */
            JSplitPane left_splitter, right_splitter;
            left_splitter  = new JSplitPane( JSplitPane.HORIZONTAL_SPLIT,
                                             false, left_panel, center_panel );
            //  left_splitter.setResizeWeight( 0.0d );
            right_splitter = new JSplitPane( JSplitPane.HORIZONTAL_SPLIT,
                                            false, left_splitter, right_panel );
            right_splitter.setOneTouchExpandable( true );
            err_msg = null;
            try {
                left_splitter.setOneTouchExpandable( true );
                right_splitter.setResizeWeight( 1.0d );
            } catch ( NoSuchMethodError err ) {
                err_msg =
                  "Method JSplitPane.setResizeWeight() cannot be found.\n"
                + "This indicates you are running an older Java2 RunTime,\n"
                + "like the one in J2SDK 1.2.2 or older. If this is the case,\n"
                + "some features in Statline window may not work correctly,\n"
                + "For instance, automatic resize of the Statline canvas\n"
                + "during window resizing and auto-update of the canvas\n"
                + "after adjustment of the slider's knob may fail silently,\n"
                + "manuel refresh of the canvas will be needed.";
            }

        this.add( right_splitter, BorderLayout.CENTER );

            /* The ToolBar for various user controls */
            toolbar = new StatlineToolBar( root_window, stat_canvas_vport,
                                           y_scrollbar, y_tree, y_maps,
                                           time_scrollbar, time_model,
                                           row_adjs );

        this.add( toolbar, BorderLayout.NORTH );

        /*
            Initialize the YaxisTree properties as well its display size which
            indirectly determines the size of CanvasTime
        */
            y_tree.init();
            row_adjs.initYLabelTreeSize();
            status_panel.addRedrawListener(
                         toolbar.getPropertyRefreshButton() );
            summary_state_combobox.addRedrawListener(
                                   toolbar.getPropertyRefreshButton() );
    }

    public void init()
    {
        // Initialize toolbar after creation of YaxisTree view
        toolbar.init();
        row_adjs.initSlidersAndTextFields();
        /*
            status_panel & summary_state_combobox listen canvas redraw events,
            They cannot be initialized before RowAdjustment has been
            initialized, i.e. before rumber of timelines has been set in
            the JTextField.
        */
        status_panel.init();
        summary_state_combobox.init();
        if ( err_msg != null )
            Dialogs.error( root_window, err_msg );

        if ( Debug.isActive() ) {
            Debug.println( "StatlinePanel.init(): time_model = "
                         + time_model );
            Debug.println( "StatlinePanel.init(): time_scrollbar = "
                         + time_scrollbar );
            Debug.println( "StatlinePanel.init(): time_ruler = "
                         + time_ruler );
        }
    }
}
